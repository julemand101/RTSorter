#!/bin/bash
file="madeleine.rxe"
nxtname=$1 #choose between "bob" or "kaare" by sending argument from eclipse external tool config

echo "Uploading "$file "to "$nxtname
gksudo -g "/home/s502a/Downloads/bricxcc1/NeXTTool /COM=$nxtname -download="$file

echo "Running program (your uploaded file should be here, and a tone should play):"

gksudo -g "/home/s502a/Downloads/bricxcc1/NeXTTool /COM=$nxtname -listfiles="$file
gksudo -g "/home/s502a/Downloads/bricxcc1/NeXTTool /COM=$nxtname -playtone=800"
gksudo -g "/home/s502a/Downloads/bricxcc1/NeXTTool /COM=$nxtname -run="$file

echo "Done Madeleine"
