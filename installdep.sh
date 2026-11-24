#!/bin/bash
set -e
echo "--Downloading dependencies for build MaXOS--"
echo "This program install dependencies, who need for MaXOS successfully build."

# first, what's name of distribution
if [ -f /etc/os-release ]; then
	. /etc/os-release
	DISTRO_ID=$ID
	echo "Distribution: $DISTRO_ID."
else
	echo "Error, /etc/os-release not found."
	exit 1
fi

# second, sudo/doas or root?
if [ "$(id -u)" -eq 0 ]; then
	PRIV_CMD=""
elif command -v sudo >/dev/null 2>&1; then
	PRIV_CMD="sudo"
elif command -v doas >/dev/null 2>&1; then
	PRIV_CMD="doas"
else
	echo "Run script at root."
	exit 1
fi
echo "Using privilege tool: ${PRIV_CMD:-"(none, already root)"}"

case "$DISTRO_ID" in
	ubuntu|debian|linuxmint)
		echo ":: Starting full system update..."
		$PRIV_CMD apt update -y
		echo "!! Using apt for install dependencies..."
		echo ":: Installing compilation collection (nasm, build-essential)"
		$PRIV_CMD apt install build-essential nasm -y
		echo ":: Installing qemu-system-x86_64"
		$PRIV_CMD apt install qemu-system-x86 -y
		;;
	arch|manjaro|cachyos|endeavouros)
		echo ":: Starting full system update..."
		$PRIV_CMD pacman -Syu --needed --noconfirm
		echo "!! Using pacman for install dependencies..."
		echo ":: Installing compilation collection (nasm, base-devel)"
		$PRIV_CMD pacman -S base-devel nasm --needed --noconfirm
		echo ":: Installing qemu-system-x86"
		$PRIV_CMD pacman -S qemu-system-x86 --needed --noconfirm
		;;
	alpine)
		echo ":: Starting full system update..."
		$PRIV_CMD apk update
		echo "!! Using apk for install dependencies..."
		echo ":: Installing compilation collection (gcc, nasm, build-base)"
		$PRIV_CMD apk add build-base gcc musl-dev nasm
		echo ":: Installing qemu-system-x86_64"
		$PRIV_CMD apk add qemu-system-x86_64
		;;
	*)
		echo "Incorrect distribution name: $DISTRO_ID"
		exit 1
		;;
esac

echo "Installed:"
echo " - build-base/base-devel/build-essential"
echo " - gcc"
echo " - nasm"
echo " - qemu-system-x86"

echo "Done."
echo "To run build, use: 'make'."
