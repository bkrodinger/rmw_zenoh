// Copyright 2023 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <signal.h>
#include <unistd.h>

#include <initializer_list>
#include <functional>
#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

#include <zenoh.h>

#include <ament_index_cpp/get_package_share_directory.hpp>

#include "../detail/zenoh_config.hpp"
#include "../detail/liveliness_utils.hpp"

#include "rmw/error_handling.h"

static bool running = true;
static sigset_t   signal_mask;

#ifdef _WIN32
BOOL WINAPI quit(DWORD ctrl_type)
{
  (void)ctrl_type;
  running = false;
  return true;
}
#else
void quit(int sig)
{
  (void)sig;
  running = false;
}
#endif

void *signal_thread(void *arg) {
  int sig_caught;    
  int rc;            

  rc = sigwait (&signal_mask, &sig_caught);
  if (rc != 0) {
      printf("wooooops\n");
  }
  switch (sig_caught)
  {
  case SIGINT:
      running = false;    
      break;
  case SIGTERM: 
      running = false;
      break;
  default:      
      fprintf (stderr, "\nUnexpected signal %d\n", sig_caught);
      break;
  }
}

int main(int argc, char ** argv)
{
  (void)argc;
  (void)argv;

  pthread_t sig_thread;
  int rc;

  sigemptyset (&signal_mask);
  sigaddset (&signal_mask, SIGINT);
  sigaddset (&signal_mask, SIGTERM);
  pthread_sigmask(SIG_BLOCK, &signal_mask, nullptr);


  rc = pthread_create(&sig_thread, NULL, signal_thread, NULL);
  if (rc != 0)
    printf("woops\n");
  

  // Initialize the zenoh configuration for the router.
  z_owned_config_t config;
  if ((rmw_zenoh_cpp::get_z_config(
      rmw_zenoh_cpp::ConfigurableEntity::Router,
      &config)) != RMW_RET_OK)
  {
    RMW_SET_ERROR_MSG("Error configuring Zenoh router.");
    return 1;
  }

  z_owned_session_t s = z_open(z_move(config));
  if (!z_check(s)) {
    printf("Unable to open router session!\n");
    return 1;
  }

    printf(
    "Started Zenoh router with id %s.\n",
    rmw_zenoh_cpp::liveliness::zid_to_str(z_info_zid(z_session_loan(&s))).c_str());

  while(running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  z_close(z_move(s));
  return 0;
}
