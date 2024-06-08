# Copyright 2021-present Maximilian Gerhardt <maximilian.gerhardt@rub.de>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
from os.path import join, isfile

from SCons.Script import DefaultEnvironment, SConscript

env = DefaultEnvironment()
core = env.BoardConfig().get("build.core", "arduino")
build_script = ""

# select build script as either from the Earle Philhower core or
# from the builder script contained in this platform.

if core == "earlephilhower":
    build_script = join(
        env.PioPlatform().get_package_dir("framework-arduinopico"), "tools", "platformio-build.py")
elif core == "kevin":
    build_script = join(
        env.PioPlatform().get_package_dir("framework-arduinopicousb"), "tools", "platformio-build.py")
else:
    build_script = join(env.PioPlatform().get_dir(), "builder",
                        "frameworks", "arduino", "mbed-core", "arduino-core-mbed.py")

if not isfile(build_script):
    sys.stderr.write(
        "Error: Missing PlatformIO build script %s!\n" % build_script)
    env.Exit(1)

SConscript(build_script)
