modmeter.lv2 - Audio Level Meter for the MOD
============================================

Install
-------

Compiling modmeter.lv2 requires the LV2 SDK, gnu-make, and a c-compiler.

```bash
  git clone git://github.com/x42/modmeter.lv2.git
  cd mclk.lv2
  make
  sudo make install PREFIX=/usr
```

To build the the MOD GUI use `make MOD=1`
