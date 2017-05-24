git clone https://github.com/gthparch/macsim ../../macsimComponent
cd ../../macsimComponent
git checkout 2bdf916fbb5e96d8f22dc7364bfa9a90110fb4b0
patch -p0 < ../CramSim/tests/patches/macsim.patch
