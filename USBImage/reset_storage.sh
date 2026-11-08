#!/bin/bash
hdiutil detach disk5
dd if=/dev/zero of=300MUSB_ehci.img bs=512 count=614400
sgdisk -n 1:2048:200M -n 2:: 300MUSB_ehci.img
hdiutil attach -nomount 300MUSB_ehci.img
