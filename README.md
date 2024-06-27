# ignore this readme for now

./a.exe | ffmpeg -y -f rawvideo -pix_fmt rgb24 -video_size 2048x1024 -i - o.png

ffmpeg -hide_banner -loglevel error -f pulse -i "alsa_input.pci-0000_00_14.2.analog-stereo" -f s16le -c:a pcm_s16le -ac 1 - | ./a.exe -stdin -a -oen notes.note
./a.exe -inotes notes.note -a
ffmpeg -hide_banner -loglevel error -f pulse -i "alsa_input.pci-0000_00_14.2.analog-stereo" -f s16le -c:a pcm_s16le -ac 1 - | ./a.exe -stdin -a -in note

ffmpeg -y -f pulse -i "alsa_input.pci-0000_00_14.2.analog-stereo" -framerate 30 -vaapi_device /dev/dri/renderD128 -f x11grab -video_size 1920x1080 -i :0 -vf 'hwupload,scale_vaapi=format=nv12' -c:v h264_vaapi -b:v 5M -c:a pcm_s16le -map 0 -map 1 -ac:0 1 o.mov
