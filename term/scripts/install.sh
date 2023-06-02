#/usr/bin/sh

sudo apt update

# For building kernel
sudo apt install libncurses-dev gawk flex bison openssl libssl-dev dkms libelf-dev libudev-dev libpci-dev libiberty-dev autoconf llvm

# For compile and helper
sudo apt install libstdc++6 clangd-12 clang-14

# For build system
