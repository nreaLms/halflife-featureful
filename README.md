# Half-Life SDK for Xash3D and GoldSource [![Build Status](https://travis-ci.org/FreeSlave/hlsdk-xash3d.svg)](https://travis-ci.org/FreeSlave/hlsdk-xash3d) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/FreeSlave/hlsdk-xash3d?svg=true)](https://ci.appveyor.com/project/FreeSlave/hlsdk-xash3d)

Half-Life SDK for Xash3D & GoldSource with some fixes and features that can be useful for mod makers.

## Features

### New monsters

#### Opposing Force monsters

Most Opposing Force monsters are implemented (exceptions are geneworm and pitworm bosses).

* monster_cleansuit_scientist
* monster_otis
* monster_blkop_osprey
* monster_blkop_apache
* monster_male_assassin
* monster_alien_voltigore
* monster_alien_babyvoltigore
* monster_pitdrone
* monster_shocktrooper
* monster_shockroach
* monster_human_grunt_ally
* monster_human_medic_ally (healing works a bit different from original)
* monster_human_torch_ally
* monster_gonome
* monster_zombie_barney
* monster_zombie_soldier

#### Other new monsters

* monster_babygarg - smaller version of Gargantua monster

#### Opposing Force dead monsters

* monster_cleansuit_scientist_dead
* monster_otis_dead
* monster_human_grunt_ally_dead
* monster_alien_slave_dead (also has original Half-Life dead poses)
* monster_houndeye_dead
* monster_gonome_dead
* monster_zombie_soldier_dead

#### Other dead monsters

* monster_male_assassin_dead (was not in Opposing Force)
* monster_human_medic_ally_dead (was not in Opposing Force)
* monster_human_torch_ally_dead (was not in Opposing Force)
* monster_alien_grunt_dead

### Monster features

* Health, relationship class, blood color, gibs and model can be customized in map editor.
* monster_barnacle health can be configured via skill.cfg
* Houndeye squad leader can play leaderlook animation.
* Alien grunts, bullsquids, houndeyes, gonomes, pitdrones and voltigores restore health when they eat meat or enemy corpses.

### New weapons

#### Opposing Force weapons

All Opposing Force weapons and corresponding ammo entities are implemented, but they may work a bit different.

* weapon_pipewrench
* weapon_knife (also has an alternative attack)
* weapon_grapple (no prediction, not tested in multiplayer)
* weapon_eagle
* weapon_penguin
* weapon_m249
* weapon_sniperrifle
* weapon_displacer
* weapon_sporelauncher
* weapon_shockrifle (plays idle animations)

#### Other new weapons

* weapon_medkit - TFC-like medkit that allows to heal allies

### Other new entities

#### Opposing Force entities

* env_spritetrain
* item_generic
* monster_skeleton_dead
* op4mortar
* trigger_playerfreeze

#### Decay entities

* item_eyescanner
* item_healthcharger
* item_recharge

#### Others

* env_warpball - easy way to create a teleportation effect for monster spawns. Also can be set as a template for monstermaker.

### Other features

* Nightvision can be enabled instead of flashlight. Both Opposing Force and Counter Strike nightvision versions are implemented.
* Added Explosive Only and Op4Mortar only flags for func_breakable. Breakables with these flags can be destroyed only with explosive weapons and op4mortar shells respectively.
* monstermaker can have env_warpball template to automatically play teleportation effects on monster spawn.
* monstermaker can set custom health, body, skin, blood color, relationship class, gibs and model for spawned monsters.

## How to build

### CMake as most universal way

    mkdir build && cd build
    cmake ../
    make

Crosscompiling using mingw:

    mkdir build-mingw && cd build-mingw
    TOOLCHAIN_PREFIX=i686-w64-mingw32 # check up the actual mingw prefix of your mingw installation
    cmake ../ -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER="$TOOLCHAIN_PREFIX-gcc" -DCMAKE_CXX_COMPILER="$TOOLCHAIN_PREFIX-g++"

You may enable or disable some build options by -Dkey=value. All available build options are defined in CMakeLists.txt at root directory.
See below if you want to build the GoldSource compatible libraries.

See below, if CMake is not suitable for you:

### Windows

#### Using msvc

We use compilers provided with Microsoft Visual Studio 6. There're `compile.bat` scripts in both `cl_dll` and `dlls` directories.
Before running any of those files you must define `MSVCDir` variable which is the path to your msvc installation.

    set MSVCDir=C:\Program Files\Microsoft Visual Studio
    compile.bat

These scripts also can be ran via wine:

    MSVCDir="z:\home\$USER\.wine\drive_c\Program Files\Microsoft Visual Studio" wine cmd /c compile.bat

The libraries built this way are always GoldSource compatible.

There're dsp projects for Visual Studio 6 in `cl_dll` and `dlls` directories, but we don't keep them up-to-date. You're free to adapt them for yourself and try to import into the newer Visual Studio versions.

#### Using mingw

TODO

### Linux

    (cd dlls && make)
    (cd cl_dll && make)

### OS X

Nothing here.

### FreeBSD

    (cd dlls && gmake CXX=clang++ CC=clang)
    (cd cl_dll && gmake CXX=clang++ CC=clang)

### Android

Just typical `ndk-build`.
TODO: describe what it is.

### Building GoldSource-compatible libraries

To enable building the goldsource compatible client library add GOLDSOURCE_SUPPORT flag when calling cmake:

    cmake .. -DGOLDSOURCE_SUPPORT=ON

or when using make without cmake:

    make GOLDSOURCE_SUPPORT=1

Unlike original client by Valve the resulting client library will not depend on vgui or SDL2 just like the one that's used in FWGS Xash3d.

Note for **Windows**: it's not possible to create GoldSource compatible libraries using mingw, only msvc builds will work.

Note for **Linux**: GoldSource requires libraries (both client and server) to be compiled with libstdc++ bundled with g++ of major version 4 (versions from 4.6 to 4.9 should work).
If your Linux distribution does not provide compatible g++ version you have several options.

#### Method 1: Statically build with c++ library

This one is the most simple but has a drawback.

    cmake ../ -DGOLDSOURCE_SUPPORT=ON -DCMAKE_C_FLAGS="-static-libstdc++ -static-libgcc"

The drawback is that the compiled libraries will be larger in size.

#### Method 2: Build in Steam Runtime chroot

This is the official way to build Steam compatible games for Linux.

Clone https://github.com/ValveSoftware/steam-runtime and follow instructions https://github.com/ValveSoftware/steam-runtime#building-in-the-runtime

    sudo ./setup_chroot.sh --i386

Then use cmake and make as usual, but prepend the commands with `schroot --chroot steamrt_scout_i386 --`:

    mkdir build-in-steamrt && cd build-in-steamrt
    schroot --chroot steamrt_scout_i386 -- cmake ../ -DGOLDSOURCE_SUPPORT=ON
    schroot --chroot steamrt_scout_i386 -- make

#### Method 3: Create your own chroot with older distro that includes g++ 4.

Use the most suitable way for you to create an old distro 32-bit chroot. E.g. on Debian (and similar) you can use debootstrap.

    sudo debootstrap --arch=i386 jessie /var/chroot/jessie-debian-i386 # On Ubuntu type trusty instead of jessie
    sudo chroot /var/chroot/jessie-debian-i386

Inside chroot install cmake, make, g++ and libsdl2-dev. Then exit the chroot.

On the host system install schroot. Then create and adapt the following config in /etc/schroot/chroot.d/jessie.conf (you can choose a different name):

```
[jessie]
type=directory
description=Debian jessie i386
directory=/var/chroot/debian-jessie-i386/
users=yourusername
groups=yourusername
root-groups=root
preserve-environment=true
personality=linux32
```

Insert your actual user name in place of `yourusername`. Then prepend any make or cmake call with `schroot -c jessie --`:

    mkdir build-in-chroot && cd build-in-chroot
    schroot --chroot jessie -- cmake ../ -DGOLDSOURCE_SUPPORT=ON
    schroot --chroot jessie -- make

#### Method 4:  Install the needed g++ version yourself

TODO: describe steps.

#### Configuring Qt Creator to use toolchain from chroot

Create a file with the following contents anywhere:

```sh
#!/bin/sh
schroot --chroot steamrt_scout_i386 -- cmake "$@"
```

Make it executable.
In Qt Creator go to `Tools` -> `Options` -> `Build & Run` -> `CMake`. Add a new cmake tool and specify the path of previously created file.
Go to `Kits` tab, clone your default configuration and choose your CMake tool there.
Choose the new kit when opening CMakeLists.txt.
