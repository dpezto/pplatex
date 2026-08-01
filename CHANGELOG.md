# Changelog

## [1.3.0](https://github.com/dpezto/pplatex/compare/v1.2.0...v1.3.0) (2026-08-01)


### Features

* **hints:** carry advice into the classic layout ([fa2afa1](https://github.com/dpezto/pplatex/commit/fa2afa13225e30593ac2d9cf73364db681079792))
* **hints:** carry advice into the classic layout ([a50f7cd](https://github.com/dpezto/pplatex/commit/a50f7cd032500c0d1ce2f4833eb5c11aa7283e12))


### Bug Fixes

* **render:** drop the rule between the source line and the notes below it ([8a7a621](https://github.com/dpezto/pplatex/commit/8a7a6211fdbc36672f5d2a6cbbecca5fa6f6dd4c))
* **render:** drop the rule between the source line and the notes below it ([e866920](https://github.com/dpezto/pplatex/commit/e866920ca62ed5dbf56cdc67d002d002f5f79724))

## [1.2.0](https://github.com/dpezto/pplatex/compare/v1.1.0...v1.2.0) (2026-07-31)


### Features

* **filter:** record which packages and classes the document loaded ([d26a3c7](https://github.com/dpezto/pplatex/commit/d26a3c74f34f53acdb3c3a882f866172431833be))
* **filter:** recover the source line and column from TeX's error context ([9351950](https://github.com/dpezto/pplatex/commit/93519502cf68c75e118d9484fd5cc11922bfcd49))
* **filter:** report a mistake once, with what followed from it ([e3def2a](https://github.com/dpezto/pplatex/commit/e3def2a8eb3c42cb45e14b9de2d1d238e47cee24))
* **hints:** say what to do about a message, and only when it is known ([5417f0a](https://github.com/dpezto/pplatex/commit/5417f0a81139aa6a7cc91f69234170a5371120e8))
* **output:** add presentation options and keep the untrimmed log line ([0482e8e](https://github.com/dpezto/pplatex/commit/0482e8e6a3aae93d702278de06a1df1e39c35e2c))
* **output:** render messages the way a compiler does ([dae5a99](https://github.com/dpezto/pplatex/commit/dae5a99029d2556bef2dd6ac0feff5fb9413692c))
* readable diagnostics, verified hints, and one message per mistake ([b20f59a](https://github.com/dpezto/pplatex/commit/b20f59a0690966c8b5761b7b32fdc457d3e07ae5))

## [1.1.0](https://github.com/dpezto/pplatex/compare/v1.0.0...v1.1.0) (2026-07-30)


### Features

* **build:** install pplatex and its engine aliases ([fafb6ba](https://github.com/dpezto/pplatex/commit/fafb6bac4ee911074221e620ebeafd052ba702ac))
* parse errors in file:line:error style ([0b74005](https://github.com/dpezto/pplatex/commit/0b74005b970459a84453600124fc3ac0d83cbd4c))


### Bug Fixes

* **build:** create engine symlinks under the install-time prefix ([e8a1b39](https://github.com/dpezto/pplatex/commit/e8a1b3941f1db039f2639cdf9af04733c97cdec8))
* **build:** migrate from PCRE1 to PCRE2 ([25c92e4](https://github.com/dpezto/pplatex/commit/25c92e437c133c11e90b041905e26afcc3427cd4))
* **build:** raise minimum CMake version to 3.20 ([0b9cd1e](https://github.com/dpezto/pplatex/commit/0b9cd1e941cd0485b11c45eb603c92382b5f7cdc))
* **build:** report the release version in CMake builds ([0679578](https://github.com/dpezto/pplatex/commit/067957872a23844a01d509075057d13c88ff1909))
* correct the verbose input message and option guard ([46a3131](https://github.com/dpezto/pplatex/commit/46a3131b588a0ed12e004fdb154bd50c77bb47a2))
* drop filler lines that are padded with trailing spaces ([b2f19e4](https://github.com/dpezto/pplatex/commit/b2f19e4d68525ae9d565b29445beac448b3d85e1))
* extract the line number from multi-line warnings ([20a99eb](https://github.com/dpezto/pplatex/commit/20a99eb53e46802117527392c480e78b97ef4ff4))
* keep the file stack balanced when a filename cannot be confirmed ([a63b56e](https://github.com/dpezto/pplatex/commit/a63b56e680de94033117a1af8b9b364286cc23ce))
* keep the whole error message when it contains a colon ([01e9bb3](https://github.com/dpezto/pplatex/commit/01e9bb39cb4e1e859eb52349cb793bf80d983a43))
* keep the whole warning message when it contains a colon ([40c50b5](https://github.com/dpezto/pplatex/commit/40c50b50820e9484e72feaf47a0e03ad3a43fd42))
* removed executable bit from source code ([102408d](https://github.com/dpezto/pplatex/commit/102408d0371948e11afe7f83c47939af3a4cecaa))
* removed executable bit from source code ([b91d4c1](https://github.com/dpezto/pplatex/commit/b91d4c1fa8a7d74fdfd8dab380cac85da4893f6d))
* select the LaTeX engine without crashing on the invoked name ([53e1131](https://github.com/dpezto/pplatex/commit/53e11317bf9cf52ef8a9d0568f3e894f269d649b))
