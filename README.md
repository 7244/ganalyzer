# ignore this readme for now

./a.exe | ffmpeg -y -f rawvideo -pix_fmt rgb24 -video_size 2048x1024 -i - o.png

ffmpeg -hide_banner -loglevel error -f pulse -i "alsa_input.pci-0000_00_14.2.analog-stereo" -f s16le -c:a pcm_s16le -ac 1 - | ./a.exe -infd -a -in note
