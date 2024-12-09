# xscreenshooter
An xfce4-screenshooter inspired screenshot utility with added ability to add your favorite file hosting site(s) as an option to upload your screenshot to.

# Compile
```
$ tar -xvf xscreenshooter.tar
$ cd xscreenshooter
$ mkdir build
$ cd build
$ cmake ..
$ make -si
```
# Usage
### GUI
```
$ ./xscreenshooter
```
### CLI
See `$ ./xscreenshooter --help`

# Adding a file hosting site
Create a `website.host` (all small) file inside the `hosts` directory with the following format:
```
file_parameter = <file>
time_parameter = <time_option>
time_options = xh,yh,zh
[key] = [value]
```
where:
- `<file>` and `<time_option>` are placeholders for the file to upload and for how long the file will be on the host's servers (if applicable).
- `time_options = xh,yh,zh` is a list of time options for how long the file will be on the host's servers (if applicable).
 - the `[key] = [value]` pairs are other parameters and arguments for POSTing to the website.
### Example
_for litterbox_
```
url = https://litterbox.catbox.moe/resources/internals/api.php
fileToUpload = <file>

reqtype = fileupload
time = <time_option>
time_options = 1h,12h,24h,72h
```
### Optional
Add a `website.favi` file to get an icon for the website on the dropdown menu.
