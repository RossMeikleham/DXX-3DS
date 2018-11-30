cd ..
make
cd 3ds_data
cp ../d1x-3ds.elf .
bannertool makebanner -i banner.png -a jingle.wav -o banner.bnr
bannertool makesmdh -s "D1X Rebirth" -l "Port of DXX-Switch/DXX-Rebirth" -p "Ross Meikleham" -i icon.png -o icon.icn
makerom -f cia -o d1x-3ds.cia -DAPP_ENCRYPTED=false -rsf d1x-3ds.rsf -target t -exefslogo -elf d1x-3ds.elf -icon icon.icn -banner banner.bnr
