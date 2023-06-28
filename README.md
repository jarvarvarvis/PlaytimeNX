# PlaytimeNX

An application statistics viewer (currently only app time) for the Nintendo Switch.

In the settings of the Nintendo Switch, playtime is only tracked after a game has been
installed for 10 days. This app solves this little, but somewhat annoying issue.

---

## Building

the following needs to be installed via dkp-pacman

- uam
- switch-glm
- libnx
- deko3d

```shell
sudo pacman -S uam switch-glm libnx deko3d
```

then download the repo using git (you can instead download the zip if you prefer)

```shell
git clone https://github.com/jarvarvarvis/PlaytimeNX.git
cd PlaytimeNX
```

then build with:

```shell
make -j
```

NOTE: 
This will use the maximum number of jobs your OS can spare for building the project.
If your computer is less powerful, consider limiting the number of jobs (to 4, for example):

```shell
make -j4
```

You can turn on optimizations and LTO by setting `build` to release:
```shell
make -j{cores} build=release
```

---

## Credits

Special thanks to the person who wrote untitled, which the GUI of PlaytimeNX is using.

- [untitled](https://github.com/ITotalJustice/untitled/)

