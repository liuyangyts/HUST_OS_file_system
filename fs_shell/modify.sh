# !/bin/zsh

rm obj build -rf
emake build.mak
cd build
./vfs.out
