# Manual

## Build

1. Change version in `src/main.cpp` and `src/control`

2. Configure CMake

3. Make sure `/app/bin`, `/app/lib`, and `/app/share` is clean; then build & install:

    ```shell
    cmake --build ~/Downloads/celestia/build --target install
    cmake --build ~/Downloads/CelestiaContent/build --target install
    ```

4. Verify viability:

    ```shell
    LD_LIBRARY_PATH=~/6.5.0/gcc_64/lib /app/bin/celestia-qt6 8000
    ```

## Packaging

1. Create a package and copy files:

    ```shell
    mkdir -p /tmp/celestia-kai/DEBIAN
    cp src/celestia/qt/control /tmp/celestia-kai/DEBIAN/control
    mkdir /tmp/celestia-kai/app
    cp -r /app/bin /tmp/celestia-kai/app
    cp -r /app/lib /tmp/celestia-kai/app
    cp -r /app/share /tmp/celestia-kai/app
    cd /tmp; dpkg-deb --build celestia-kai; cd -
    ```

2. Use `apt` INSTEAD OF `dpkg -i` to install package:

    ```shell
    sudo apt install /tmp/celestia-kai.deb
    ```
