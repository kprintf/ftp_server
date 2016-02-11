#!/bin/bash
sudo rm /tmp/ftpmod_config
pushd ./ > /dev/null
cd bin
sudo ./ftpmod
popd > /dev/null
