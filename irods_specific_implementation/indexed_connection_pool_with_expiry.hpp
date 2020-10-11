#ifndef IRODS_INDEXED_CONNECTION_POOL_HPP
#define IRODS_INDEXED_CONNECTION_POOL_HPP

#include "rcConnect.h"
#include "irods_server_properties.hpp"

#include "jwt.h"

#include <map>
#include <string>
#include <thread>
#include <chrono>

namespace irods {
    namespace {
        namespace keyword {
            const std::string user_name{"user_name"};
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
                rodsEnv env;
                _getRodsEnv(env);
                conn_ = _rcConnect(
                            env.rodsHost,
                            env.rodsPort,
                            env.rodsUserName,
                            env.rodsZone,
                            _client_name.c_str(),
                            env.rodsZone,
                            &err_,
                            0,
                            NO_RECONN);
            } // ctor

            connection_handle(
                  const std::string& _client_name
                , const std::string& _proxy_name)
            {
                rodsEnv env;
                _getRodsEnv(env);
                conn_ = _rcConnect(
                            env.rodsHost,
                            env.rodsPort,
                            _proxy_name.c_str(),
                            env.rodsZone,
                            _client_name.c_str(),
                            env.rodsZone,
                            &err_,
                            0,
                            NO_RECONN);
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
        time_type                 access_time;
        connection_handle_pointer connection;
    }; // connection_context

    class connection_proxy {
        connection_context& ctx_;

        public:
            connection_proxy(connection_context& _ctx) : ctx_{_ctx}
            {

            } // connection_proxy

            ~connection_proxy()
            {
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

        auto now_in_seconds() -> time_type
        {
            return std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now());
        } // now_in_seconds

        auto manage_lifetimes() -> void
        {
            while(!exit_flag_) {
                auto exp  = time_type{now_in_seconds() - max_idle_timeout_in_seconds_};
                auto itr  = pool_.begin();
                auto done = pool_.end() == itr;

                while(!done) {
                    std::scoped_lock lk(pool_mutex_);

                    if(!itr->second.in_use &&
                       exp > itr->second.access_time) {
                        // release the connection
                        pool_.erase(itr);

                        // reset the iterator and start over
                        itr = pool_.begin();
                    }
                    else {
                        ++itr;
                    }

                    done = pool_.end() == itr;

                } // while

                std::this_thread::sleep_for(sleep_time_);

            } // while

        } // manage_lifetimes

        auto get_user_name_from_key(const std::string& _jwt) -> std::string
        {
            // use the zone key as our secret
            std::string zone_key{irods::get_server_property<const std::string>(irods::CFG_ZONE_KEY_KW)};

            // decode the jwt
            auto decoded = jwt::decode(_jwt);

            // verify the jwt
            auto verifier = jwt::verify()
                                .allow_algorithm(jwt::algorithm::hs256{zone_key})
                                .with_issuer(keyword::issue_claim);
            verifier.verify(decoded);

            auto payload = decoded.get_payload_claims();

            return payload[keyword::user_name].as_string();

        } // get_user_name_from_key

        auto make_connection(const std::string& _key) -> std::shared_ptr<connection_handle>
        {
            return std::make_shared<connection_handle>(get_user_name_from_key(_key));

        } // make_connection

        public:

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

             auto get(const std::string& _key) -> connection_proxy
             {
                 std::scoped_lock lk(pool_mutex_);

                 auto& ctx = pool_[_key];

                 ctx.in_use = true;

                 if(!ctx.connection.get()) {
                     ctx.connection = make_connection(_key);
                 }

                 ctx.access_time = now_in_seconds();

                 pool_[_key] = ctx;

                 return connection_proxy{ctx};

             } // get

    }; // indexed_connection_pool_with_expiry

} // namespace irods

#endif  // IRODS_INDEXED_CONNECTION_POOL_HPP
