import json
import sys

def init():

    return './jenkins_testing'  # --> repository relative path to docker-compose.yml

def run (CI):

    final_config = CI.store_config(
        {
            "build_services_in_order": [
                "package_copier",
                "package_builder"
            ],

            "up_service": "client_runner",

            "yaml_substitutions": {       # -> written to ".env"

                # Default typographical substitutions within docker-compose.yml; these can be
                # overridden in the CONFIG_JSON parameter of the Jenkins client dot:

                "os_platform": "ubuntu_18",     # used to select among Dockerfiles
                "os_version":  "ubuntu:18.04"   # used to determine exact OS. make this consistent with "os_platform",
                                                #  ie. "centos:7" for "centos_7" and "ubuntu:XY.04" for "ubuntu_XY"
            },

            "container_environments": {
                "client-runner" : {       # -> written to "client-runner.env"
                }
            }
        }
    )

    print ('----------\nconfig after CI modify pass\n----------',file=sys.stderr)
    print(json.dumps(final_config,indent=4),file=sys.stderr)

    return CI.run_and_wait_on_client_exit ()
