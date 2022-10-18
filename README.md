# ThinROS for Linux Backend

## How to build the kernel module?

### TX2

On a TX2, first you need to get cmake version greater than 3.13. You will likely need to download a fresh version as the version on the apt repository is too old.

Once cmake is installed, run the following configure command.

```shell
mkdir -p build
cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=../target-linux-aarch64-tx2-gnu.cmake
```

Then build and install the driver

```shell
make driver
make install_driver
```

## How to use the kernel module?

1. patch the kernel device tree

```shell
dtc -I dtb -O dts -o tegra186.dts ~/nvidia/nvidia_sdk/JetPack_4.5.1_Linux_JETSON_TX2/Linux_for_Tegra/kernel/dtb/tegra186-quill-p3310-1000-c03-00-base.dtb

bash patch-dts.sh

dtc -I dts -O dtb -o ~/nvidia/nvidia_sdk/JetPack_4.5.1_Linux_JETSON_TX2/Linux_for_Tegra/kernel/dtb/tegra186-quill-p3310-1000-c03-00-base.dtb tegra186.dts
```

2. flash the device tree

```shell
sudo ./flash.sh -r -k kernel-dtb jetson-tx2 external
```

3. use the driver

```shell
# load the driver
make install

# unload the driver
make uninstall
```

see `thinros_test.c`

