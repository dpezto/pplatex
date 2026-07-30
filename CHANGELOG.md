# Changelog

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
