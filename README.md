I Have forked this repo to add a few key pieces of functionality that I need for live use of Snapshots

- Tracks Safes (tracks that are excluded from the save and recall of snapshots)
- Record Functionality, by default the sws snapshots do not fully recall scenes when there is a recording running. To combat this, I have it stop the recording, recall the scene, then restart the recording
- Snapshots, there is now an optional area to tie together snapshots and screensets so everything recalls at once.

This is my first endeavor with cpp and none of the code here is guaranteed. Some were helped along by chatbot. I tried to mark all of my additions with comments such as //William for if anyone wants to fix this up for their own use. 


# SWS Extension

[![Build status](https://ci.appveyor.com/api/projects/status/6jq0uwut3mx14xp4/branch/master?svg=true)](https://ci.appveyor.com/project/reaper-oss/sws/branch/master)

The SWS extension is a collection of features that seamlessly integrate into
REAPER, the Digital Audio Workstation (DAW) software by Cockos, Inc.

For more information on the capabilities of the SWS extension or to download it,
please visit the [end-user website](https://www.sws-extension.org).
For more information on REAPER itself or to download the latest version,
go to [reaper.fm](https://www.reaper.fm).

You are welcome to contribute! Just fork the repo and send us a pull request
(also see [Submitting pull requests](https://github.com/reaper-oss/sws/wiki/Submitting-pull-requests)
in the [wiki](https://github.com/reaper-oss/sws/wiki)).
See [Building the SWS Extension](https://github.com/reaper-oss/sws/wiki/Building-the-SWS-Extension)
for build instructions.

While not strictly required by the license, if you use the code in your own
project we would like definitely like to hear from you.

The SWS extension includes the following 3rd party libraries (which are all
similarly licensed):

- [OscPkt](http://gruntthepeon.free.fr/oscpkt/)
- [libebur128](https://github.com/jiixyj/libebur128)

The SWS extension also relies on [TagLib](https://taglib.org/) and
[WDL](https://www.cockos.com/wdl).
