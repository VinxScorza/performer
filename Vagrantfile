# -*- mode: ruby -*-
# vi: set ft=ruby :

$provision_script = <<-'SCRIPT'

set -eu

echo "Installing dependencies ..."
sudo apt-get update
sudo apt-get install -y git python python3 python3-dev build-essential libtool autoconf cmake libusb-1.0.0-dev libftdi-dev pkg-config wget ca-certificates

echo "Installing CMake 3.26.4 ..."
(
    cmake_tmp_dir=$(mktemp -d)
    trap 'rm -rf "$cmake_tmp_dir"' EXIT
    cd "$cmake_tmp_dir"

    cmake_installer=cmake-3.26.4-linux-x86_64.sh
    # Official checksum: https://github.com/Kitware/CMake/releases/download/v3.26.4/cmake-3.26.4-SHA-256.txt
    cmake_sha256=413e59e94b9a3eed2d73f8fc85520d505e514e95005471504d8bebe844970d67
    wget --https-only --timeout=30 --tries=3 -O "$cmake_installer" "https://github.com/Kitware/CMake/releases/download/v3.26.4/$cmake_installer"
    printf '%s  %s\n' "$cmake_sha256" "$cmake_installer" > installer.sha256
    sha256sum --check installer.sha256
    sudo sh "$cmake_installer" --skip-license --prefix=/usr/local
)
hash -r
cmake --version
python3 --version

echo "Installing toolchain ..."
cd /vagrant
export TOOLS_DIR=/home/vagrant/tools
make tools_install

echo "Setting up build ..."
make setup_stm32

echo "Setting up programmers ...."
# Allow non-root users to access USB devices
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="3748", GROUP="users", MODE="0666"' >> /etc/udev/rules.d/60-programmers.rules
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="15ba", ATTRS{idProduct}=="0003", GROUP="users", MODE="0666"' >> /etc/udev/rules.d/60-programmers.rules
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="15ba", ATTRS{idProduct}=="002a", GROUP="users", MODE="0666"' >> /etc/udev/rules.d/60-programmers.rules
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="15ba", ATTRS{idProduct}=="002b", GROUP="users", MODE="0666"' >> /etc/udev/rules.d/60-programmers.rules
echo 'SUBSYSTEMS=="usb", ATTRS{idVendor}=="16c0", ATTRS{idProduct}=="05dc", GROUP="users", MODE="0666"' >> /etc/udev/rules.d/60-programmers.rules
echo 'SUBSYSTEMS=="usb", KERNEL=="ttyUSB*", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", GROUP="users", MODE="0666", SYMLINK+="ftdi-usbserial"' >> /etc/udev/rules.d/60-programmers.rules
sudo udevadm control --reload-rules
sudo udevadm trigger

echo "Setting up environment ..."
echo "export TOOLS_DIR=/home/vagrant/tools" >> /home/vagrant/.bashrc
echo "source /vagrant/bin/activate" >> /home/vagrant/.bashrc
echo "cd /vagrant" >> /home/vagrant/.bashrc

SCRIPT

Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/xenial64"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder "../data", "/vagrant_data"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  config.vm.provider "virtualbox" do |vb|
   # Display the VirtualBox GUI when booting the machine
   # vb.gui = true

   # Customize the amount of memory on the VM:
   vb.memory = "4096"
   
   # Use 2 CPUs
   vb.cpus = 2
   
   # Enable all USB devices needed for development
   vb.customize ['modifyvm', :id, '--usb', 'on']
   vb.customize ['modifyvm', :id, '--usbehci', 'on']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'STLink', '--vendorid', '0x0483', '--productid', '0x3748']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'Olimex OpenOCD JTAG ARM-USB-TINY-H', '--vendorid', '0x15BA', '--productid', '0x002a']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'Olimex OpenOCD JTAG ARM-USB-OCD-H', '--vendorid', '0x15BA', '--productid', '0x002b']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'Olimex OpenOCD JTAG', '--vendorid', '0x15BA', '--productid', '0x0003']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'FTDI serial adapter', '--vendorid', '0x0403', '--productid', '0x6001']
   vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', 'USBasp', '--vendorid', '0x16c0', '--productid', '0x05dc']
  end

  # provisioning scripts
  config.vm.provision :shell, inline: $provision_script
end
