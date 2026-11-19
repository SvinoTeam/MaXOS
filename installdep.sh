#!/bin/bash
set -e
echo "--Downloading dependencies for build MaXOS--"
echo "This program install dependencies, who need for MaXOS successfully build."

# first, what's name of distribution
if [ -f /etc/os-release ]; then
	. /etc/os-release
	OS=$ID
	echo "Distribution: $OS."
else
	echo "Error, /etc/os-release not found."
	exit 1
fi

# second, sudo/doas or root?
if [ "$(id -u)" -eq 0 ]; then
	PRIV_CMD=""
elif command -v sudo >/dev/null 2>&1; then
	PRIV_COMMAND="sudo"
elif command -v doas >/dev/null 2>&1; then
	PRIV_CMD="doas"
else
	echo "Run script at root."
	exit 1
fi
echo "Using privilege tool: ${PRIV_CMD:-"(none, already root)"}"

case "$ID" in
	ubuntu|debian|linuxmint)
		echo "Starting full system update..."
		$PRIV_CMD apt update -y 
		echo "Using apt for install dependencies..."
		echo "Installing compilation collection (nasm, build-essential)"
		$PRIV_CMD apt install build-essential nasm -y
		echo "Installing qemu-system-x86_64"
		$PRIV_CMD apt install qemu-system-x86 -y
		;;
	arch|manjaro|cachyos|endeavouros)
		echo "Starting full system update..."
		$PRIV_CMD pacman -Syu --needed --noconfirm
		echo "Using pacman for install dependencies..."
		echo "Installing compilation collection (nasm, base-devel)"
		$PRIV_CMD pacman -S base-devel nasm --needed --noconfirm
		echo "Installing qemu-full"
		$PRIV_CMD pacman -S qemu-full --needed --noconfirm
		;;
	alpine)
		echo "Starting full system update..."
		$PRIV_CMD apk update
		echo "Using apk for install dependencies..."
		echo "Installing compilation collection (gcc, nasm, build-base)"
		$PRIV_CMD apk add build-base gcc musl-dev nasm
		echo "Installing qemu-system-x86_64"
		$PRIV_CMD apk add qemu-system-x86_64
		;;
	*)
		echo "Incorrect distribution name: $ID"
		exit 1
		;;
esac

echo "Installed:"
echo " - build-base/base-devel/base-essential"
echo " - gcc"
echo " - nasm"
echo " - qemu-system-x86_64"

echo "Done."
echo "To run build, use: 'make'."

