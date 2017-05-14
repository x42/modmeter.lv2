modmeter.lv2 - Audio Level Meter for the MOD
============================================

A simple audio meter with numeric display for http://moddevices.com/
constsing of a Digital Peak Meter with 15dB/sec falloff with user-resettable
peak-hold and a RMS loudness meter (600ms)

Install
-------

Compiling modmeter.lv2 requires the LV2 SDK, gnu-make, and a c-compiler.

```bash
  git clone git://github.com/x42/modmeter.lv2.git
  cd modmeter.lv2
  make
  sudo make install PREFIX=/usr
```

To build the the MOD GUI use `make MOD=1`
