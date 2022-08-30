#ifndef IRODS_INDEXED_CONNECTION_POOL_HPP
#define IRODS_INDEXED_CONNECTION_POOL_HPP

#include "configuration.hpp"

#include <irods/irods_exception.hpp>
#include <irods/irods_random.hpp>
#include <irods/obf.h>
#include <irods/rcConnect.h>
#include <irods/rodsErrorTable.h>

#include "jwt.h"
#include <fmt/format.h>

#include <map>
#include <string>
#include <thread>
#include <chrono>

namespace irods {
    namespace {
        namespace keyword {
            const std::string user_name{"user_name"};
            const std::string password{"password"};
            const std::string issue_claim{"issue_claim"};
            const std::string subject_claim{"subject_claim"};
            const std::string audience_claim{"audience_claim"};
        } // namespace keyword

        const uint32_t default_idle_time_in_seconds{10};

    } // namespace

    class connection_handle {
        public:
            connection_handle() : conn_{nullptr}
            {
                // ctor
            }

            connection_handle(const std::string& _client_name)
            {
                const auto& env = irods::rest::configuration::irods_client_environment();
                const char* host = env.at("host").get_ref<const std::string&>().data();
                const auto port = env.at("port").get<int>();
                const char* zone = env.at("zone").get_ref<const std::string&>().data();
                const char* rodsadmin_username = env.at("rodsadmin_username").get_ref<const std::string&>().data();
                conn_ =
                    _rcConnect(host, port, rodsadmin_username, zone, _client_name.c_str(), zone, &err_, 0, NO_RECONN);
            } // ctor

            connection_handle(
                  const std::string& _client_name
                , const std::string& _proxy_name)
            {
                const auto& env = irods::rest::configuration::irods_client_environment();
                const char* host = env.at("host").get_ref<const std::string&>().data();
                const auto port = env.at("port").get<int>();
                const char* zone = env.at("zone").get_ref<const std::string&>().data();
                conn_ =
                    _rcConnect(host, port, _proxy_name.c_str(), zone, _client_name.c_str(), zone, &err_, 0, NO_RECONN);
            } // ctor

            virtual ~connection_handle()
            {
                if(conn_) {
                    rcDisconnect(conn_);
                }
            }

            auto get() -> rcComm_t*
            {
                return conn_;
            }

        private:
            rErrMsg_t  err_;
            rcComm_t*  conn_;

    }; // class connection_handle

    using connection_handle_pointer = std::shared_ptr<connection_handle>;

    using time_type = std::chrono::time_point<std::chrono::system_clock>;

    struct connection_context
    {
        bool                      in_use;
        bool                      evict_immediately;
        time_type                 access_time;
        connection_handle_pointer connection;

        connection_context()
            : in_use{false}
            , evict_immediately{false}
        , access_time{}
        , connection{}
        {
            // ctor
        }
    }; // connection_context

    namespace {
        auto now_in_seconds() -> time_type
        {
            return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
        } // now_in_seconds
    }

    class connection_proxy {
        connection_context& ctx_;

        public:
            connection_proxy(connection_context& _ctx) : ctx_{_ctx}
            {

            } // connection_proxy

            ~connection_proxy()
            {
                ctx_.access_time = now_in_seconds();
                ctx_.in_use = false;
            }

            auto operator()() -> rcComm_t*
            {
                return ctx_.connection->get();
            }

    }; // connection_proxy

    class indexed_connection_pool_with_expiry
    {

        using sleep_type = std::chrono::duration<uint32_t>;
        using connection_pool_type = std::map<std::string, connection_context>;

        std::mutex           pool_mutex_;
        std::atomic_bool     exit_flag_{false};
        std::thread          life_time_manager_;
        time_type            max_idle_timeout_in_seconds_;
        sleep_type           sleep_time_;
        connection_pool_type pool_;

        auto manage_lifetimes() -> void
        {
            while(!exit_flag_) {
                auto exp  = time_type{now_in_seconds() - max_idle_timeout_in_seconds_};
                auto itr  = pool_.begin();
                auto done = pool_.end() == itr;

                while(!done) {
                    std::scoped_lock lk(pool_mutex_);

                    if(!itr->second.in_use) {
                        // either the connection is old, or it is not to be kept
                        if(exp > itr->second.access_time || itr->second.evict_immediately) {
                            // release the connection
                            pool_.erase(itr);

                            // reset the iterator and start over
                            itr = pool_.begin();

                            done = pool_.end() == itr;

                            continue;
                        }
                    }

                    ++itr;

                    done = pool_.end() == itr;

                } // while

                std::this_thread::sleep_for(sleep_time_);

            } // while

        } // manage_lifetimes

        auto get_user_name_from_key(const std::string& _jwt) -> std::string
        {
            // decode the jwt
            auto decoded = jwt::decode(_jwt);

            const auto& signing_key = irods::rest::configuration::get_jwt_signing_key();

            // verify the jwt
            auto verifier =
                jwt::verify().allow_algorithm(jwt::algorithm::hs256{signing_key}).with_issuer(keyword::issue_claim);
            verifier.verify(decoded);

            auto payload = decoded.get_payload_claims();

            return payload[keyword::user_name].as_string();
        } // get_user_name_from_key

        static auto save_rodsadmin_password_if_necessary() -> void
        {
            namespace irc = irods::rest::configuration;

            if (const int ec = obfGetPw(nullptr); ec < 0) {
                const auto& rodsadmin_password =
                    irc::irods_client_environment().at("rodsadmin_password").get_ref<const std::string&>();
                if (const int ec = obfSavePw(0, 0, 0, rodsadmin_password.data()); ec < 0) {
                    THROW(ec, "Failed to save password for rodsadmin proxy user");
                }
            }
        } // save_rodsadmin_password_if_necessary

        auto make_connection(const std::string& _jwt) -> std::shared_ptr<connection_handle>
        {
            auto user_name = get_user_name_from_key(_jwt);

            auto conn = std::make_shared<connection_handle>(user_name);

            // If we can't get the obfuscated password, the rodsadmin proxy user has not been authenticated.
            // All currently supported authentication plugins require the obfuscated password file to exist
            // when using clientLogin, so make sure this is done here before proceeding to authentication.
            save_rodsadmin_password_if_necessary();

            auto err = clientLogin(conn->get());
            if(err < 0) {
                THROW(err, fmt::format("[{}] failed to login", user_name));
            }

            return conn;

        } // make_connection

        auto get_random_hint() -> std::string
        {
            char bytes[32];
            irods::getRandomBytes(bytes, sizeof(bytes));

            std::string hint;
            for(auto b : bytes) {
                hint += 'a' + (b % 26);
            }

            return hint;

        } // get_random_hint

        public:

             inline static const std::string do_not_cache_hint{"DO_NOT_CACHE_HINT"};

             indexed_connection_pool_with_expiry()
             : life_time_manager_(&indexed_connection_pool_with_expiry::manage_lifetimes, this)
             , max_idle_timeout_in_seconds_(std::chrono::seconds(default_idle_time_in_seconds))
             , sleep_time_(std::chrono::seconds(default_idle_time_in_seconds/4))
             {
                 // ctor
             }

             indexed_connection_pool_with_expiry(uint32_t _it)
             : life_time_manager_(&indexed_connection_pool_with_expiry::manage_lifetimes, this)
             , max_idle_timeout_in_seconds_(std::chrono::seconds(_it))
             , sleep_time_(std::chrono::seconds(_it/4))
             {
                 // ctor
             }

             ~indexed_connection_pool_with_expiry()
             {
                 exit_flag_ = true;
                 life_time_manager_.join();
             }

             auto set_idle_timeout(uint32_t _it) -> void
             {
                 max_idle_timeout_in_seconds_ = time_type{std::chrono::seconds(_it)};
                 sleep_time_ = std::chrono::seconds(_it/4);
             }

             auto get(const std::string& _jwt, const std::string& _hint) -> connection_proxy
             {
                 auto do_not_cache_flag = (do_not_cache_hint == _hint);

                 auto key = _jwt;
                 key += do_not_cache_flag
                        ? std::string{"___"} + get_random_hint()
                        : _hint;

                 std::scoped_lock lk(pool_mutex_);

                 auto& ctx = pool_[key];

                 if(ctx.in_use) {
                     THROW(
                         SYS_USER_NOT_ALLOWED_TO_CONN,
                         "connection already in use for token");
                 }

                 ctx.in_use = true;
                 ctx.evict_immediately = do_not_cache_flag;

                 if(!ctx.connection.get()) {
                     ctx.connection = make_connection(_jwt);
                 }

                 ctx.access_time = now_in_seconds();

                 pool_[key] = ctx;

                 return connection_proxy{ctx};

             } // get

    }; // indexed_connection_pool_with_expiry

} // namespace irods

#endif  // IRODS_INDEXED_CONNECTION_POOL_HPP
