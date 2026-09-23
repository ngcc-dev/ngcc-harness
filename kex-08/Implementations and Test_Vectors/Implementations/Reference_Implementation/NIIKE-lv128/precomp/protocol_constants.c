// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#include "protocol_constants.h"

// const orient_cycle_t base_cycle = {{{{{0x3b34d36b0bf43, 0x1383447af875, 0x283ea8332f404, 0x7491c1753feaf, 0x156d8bcecb9d}, {0x9af27d85bbfe7, 0x6d80467b2b1c7, 0x11bcc48b0ef26, 0xc9adc935fdaff, 0x4ab5ce5740d}}, {{0x4}, {0x0}}}, {{{0x7a7f38ea4cea1, 0x3801e42d75fc5, 0x50a92df0d39a, 0xa1510d0895392, 0x298aef6376df}, {0x473ec06c8f336, 0x754091b981e3c, 0x9d0428c0ff627, 0x7c37cd6063aa7, 0x3fd6b8e1ca81}}, {{0x1}, {0x0}}}, {{{0x9fce27525e51, 0x5db241f9034f5, 0x41e3286a3b3d8, 0x32f0fff9c5a1, 0x1b249aec7742}, {0x9a84d6e1ac1ed, 0x18b0c20619a6a, 0x76a7eca4d534e, 0x9706d2f407dee, 0x1e71db62a608}}, {{0x1}, {0x0}}}, {{{0x8c3f078691589, 0xa1ab0017f1e75, 0xdcbfc7d2f4494, 0x74dd59cb557e5, 0x33ae2c254bc5}, {0x5f96dafc28764, 0xc1fd81455dab3, 0xb6efdb544e69d, 0xe72349408166, 0x35c2213cc84b}}, {{0x1}, {0x0}}}, {{{0x2650f4d950e26, 0x260ad12fa19a, 0x28195b2236e2, 0x46c8bc8d449a8, 0x1a1d1193fbd9}, {0xbc2ff2707465d, 0x27624769aa976, 0x13af5e594c0bc, 0x1a7c7527e8ecc, 0x49f3fda643df}}, {{0x1}, {0x0}}}},
// {{{{0xfacb588412b0, 0x2fbd02c3dea30, 0x4d77882eed108, 0x7bd2ba12c18d6, 0x85da9dc09f0}, {0xc2f246975a420, 0x68d11c1deeba2, 0x8751dedbfc61c, 0x268d6cca42aac, 0xa47da10ab5}}, {{0x4}, {0x0}}}, {{{0x779078e161ee9, 0xcfc35ba96a02f, 0x826e84fd16db1, 0x8d2d9ac5eb7eb, 0x4b12c0ded072}, {0x739d93eedd29b, 0xe34d57c2961f9, 0x76ee4fcd03ecb, 0xbe83b9ae9e931, 0x2ccfc27ded05}}, {{0x1}, {0x0}}}, {{{0x514c9f62c5fce, 0x4d6d5c078b65f, 0x65187442aca4d, 0xee304f3e17e, 0x30995632a9ae}, {0xf866071937261, 0x6ed0e6b7526c1, 0xbc668735bd7fe, 0x54f4d4a1c782a, 0xa8fbfc7ed2a}}, {{0x1}, {0x0}}}, {{{0xb45ccdab50e45, 0x6498db81190aa, 0xeb25964010af0, 0x9ffbfe92972b4, 0xc95267a71c2}, {0xe63829c5ae07c, 0xdb706d9785733, 0xc06b586964a94, 0xf5684a6296bfd, 0x2d2b376bb847}}, {{0x1}, {0x0}}}, {{{0xf1da94af11124, 0xbee9e94c07972, 0xdbbb35dab42b6, 0xe50eaf4b06b7f, 0x26ea42a44f28}, {0x2104252492216, 0xe3093114ffb83, 0xfb539d404e8bf, 0x23642c780fae1, 0x3728c303906c}}, {{0x1}, {0x0}}}},
// {{{{0x3a360130d44ab, 0x523e65891ca29, 0x5808240670fc5, 0x7c17c0d775c74, 0x5124a89a811b}, {0xb47059fef4d92, 0x1d4f356be5dca, 0x70e4a987eb8ff, 0xb0b1c14e8ca8b, 0x39f76b2c7a2a}}, {{0x4}, {0x0}}}, {{{0x96079d663ed8c, 0x4540328a9a5cb, 0x67fd827693b75, 0x3bf66fc32d3e4, 0x19dbb8141753}, {0x420caea2d9998, 0xadf0a3ee4ab8d, 0x90b5c0140afa9, 0x510c4a8eb956d, 0x44475ec95734}}, {{0x1}, {0x0}}}, {{{0x5471c5e285b6d, 0xc7fb6fff8d3a5, 0x4396c2409c33, 0x8becb61b231f7, 0x281679bc8b5e}, {0xae03c81753f87, 0xdd6c1e5454862, 0x18802c8479b6, 0xb84d83b94f1d8, 0x45eba239baa3}}, {{0x1}, {0x0}}}, {{{0x18ac72a854faf, 0x8253dd0b0f4ed, 0x36446bec8762a, 0x8beaee16ec937, 0x3fb5142e378}, {0x81528a0f82503, 0x29c26c1cd8b8b, 0x18b779aeef777, 0x7443deb147806, 0x46912e0493f2}}, {{0x1}, {0x0}}}, {{{0x129aa9b67266a, 0xd3b42a689fc0d, 0x4344df29ca163, 0xd54591ccf0e00, 0xb6af0c233d}, {0xcf0c03c971f39, 0x9155021caada3, 0xea178b2462723, 0x6f574cf58a319, 0x28e3d0ee1e44}}, {{0x1}, {0x0}}}},
// {{{{0xcd1f61c126a04, 0x429d1450a0277, 0xb420fab1edc35, 0x1d0e922e2d72c, 0x4c36323e63}, {0x341423f481888, 0xdb171eebb51fa, 0xd453371809231, 0x12978dd761768, 0x13fc0ba5d651}}, {{0x4}, {0x0}}}, {{{0xf4d5792d1636c, 0x6fdecb9ddf53d, 0xec6819496e110, 0x7f8bdabd354c6, 0x6ac5a335522}, {0xcacf165e34bf2, 0xbfbb82ae3b800, 0x8dae8cd9fe5dc, 0xe1028e89768b2, 0x3272d8517a0}}, {{0x1}, {0x0}}}, {{{0xc163a0b37928e, 0x15a22ada87237, 0xc970128374fe8, 0x2521050b09392, 0x376110664658}, {0x7b18b343773d, 0x94bc8ed1d1f96, 0x615a61a5cb8c1, 0x8c00601873d8d, 0x2df5ed813f96}}, {{0x1}, {0x0}}}, {{{0xb6104776ac9ea, 0xd5ffaf05d5058, 0x86519d553ec88, 0x58ea538554165, 0x527b15828a86}, {0x20cfaf9bdec68, 0x1376b3e727808, 0xd563b37b9691a, 0xdc7c303feb63b, 0x1eaa36519d62}}, {{0x1}, {0x0}}}, {{{0xc7281ec2ce5, 0x5b663c192f347, 0x4de98b5f91cbb, 0xa222745f01547, 0x533bf4318fd8}, {0x1dc1debe5bc13, 0x5c95975d1a446, 0xe7ec7fae5b77c, 0x1df38b400cba1, 0x7d91c4289ec}}, {{0x1}, {0x0}}}},
// {{{{0x248235c073652, 0x7f85ef05a82f0, 0x9403a7d479f2e, 0xca7b0e1b8e475, 0x3f08b1c95be4}, {0xcbd72736033cd, 0x8ff50117d023a, 0xe553d13f96460, 0x1c3b7bb6a08dd, 0x3ffcc9cd5d66}}, {{0x4}, {0x0}}}, {{{0xcbcadb75f2420, 0xaac78f9b4c358, 0x7708088700166, 0x8c1ff00b8cbe1, 0x2b551f6ad3dc}, {0x6b772f8e45610, 0x46b08ebf88fb0, 0xd2c8e00764177, 0x4e986db71df77, 0x49289db0fc37}}, {{0x1}, {0x0}}}, {{{0x2a292cf51d550, 0xbc78080d8425, 0xff52715cf6e6b, 0x68878268e6947, 0x5302d8b97bd4}, {0x35f2d66e5e59d, 0x1edee66d8e7e4, 0x138ce8a417030, 0x37aaea6ed82f8, 0x51c8b17820ce}}, {{0x1}, {0x0}}}, {{{0x3bb900e5e6c61, 0x4fbbb24e2b986, 0x43072489cdeba, 0xe061fbb3f68f4, 0x342f853915c2}, {0xbff33a0b7c3c5, 0xb3fa71904f0e4, 0xa1bb63685073c, 0x6bfe1283036fa, 0x4f46db22a363}}, {{0x1}, {0x0}}}, {{{0xad56c46210dea, 0xa92819695d08c, 0x4f9c12573e6d1, 0x6a7af6ad57d6b, 0x1922a2118822}, {0x71eabcdf58de1, 0x21dc33d6e7cd6, 0x521e90d215e0a, 0x6dec8c559cf63, 0xf99060b6e3b}}, {{0x1}, {0x0}}}},
// {{{{0x1d2b4e6775ca6, 0xd076dfa389c40, 0x17e7dd6b1dd31, 0xd3d414f54221b, 0x4b0846418eb5}, {0xddbed043cbde2, 0x32b131ed38a00, 0xfd7cd0321930e, 0x3c093036af107, 0xa97cd5736ba}}, {{0x4}, {0x0}}}, {{{0xfb8b5a641e022, 0x2236ff76077c8, 0xadcff8948e874, 0x4e5b52bbcf572, 0x3cd0790dd612}, {0x8a03bd6504815, 0xdcf75e14109dc, 0x9998ae1f08331, 0xa705d01eed984, 0x44f0fcaeb2b2}}, {{0x1}, {0x0}}}, {{{0xb145640fbbb05, 0xcdcc2a30f9d6b, 0x4efd5201dd434, 0xbf9fe33887724, 0x44a46fffd72b}, {0xbcd6f3f4fb4f7, 0xd8598bd8a9950, 0xa002a05e964ed, 0x495700e13c766, 0x15f6c13584b}}, {{0x1}, {0x0}}}, {{{0xf2c3b210a0d4a, 0x8e626047797f4, 0xe78ff1733963, 0x8250b675ce6ec, 0x35601f286470}, {0xe8c6131ffcfa6, 0x952972f9ea917, 0x950fa72eeabe4, 0x144e69fe3f9fe, 0x12c1a3ca1ab2}}, {{0x1}, {0x0}}}, {{{0x606645a412ed8, 0xa19543deee6fd, 0xece64813b41c6, 0xc7834489e08c, 0x65d0fa6e424}, {0xd7d2c4e98e929, 0xce6a893e8028a, 0x24238947f25a0, 0x652c8a406c640, 0xf15dd19d708}}, {{0x1}, {0x0}}}},
// {{{{0x856659e209999, 0x6159bb375b7f7, 0xc648c6416458d, 0xdd7558bbe5bfe, 0x3a61991b08ad}, {0x2a44bd8f02014, 0xe4bcb2475c191, 0xd46931bb44cbb, 0x5e01480f451e3, 0x346280f2a735}}, {{0x4}, {0x0}}}, {{{0x545097930b996, 0x427840bd8b29e, 0x7e272dc32db83, 0xc2ae93b711e34, 0xeeb7d3a915f}, {0x1181c4eae445b, 0x9a50270512dac, 0x754ce593d4568, 0x1152ce8eee9b, 0x221cd4931c51}}, {{0x1}, {0x0}}}, {{{0xee5ca58d160d3, 0xa1a19e65ee354, 0x78aab3f7d9b6c, 0x44db41616c4f3, 0x15be55923c40}, {0x920a73b020715, 0x18964b094849e, 0xa567ecb7ed1c4, 0xc13f0ba5cbd4b, 0x27309c7962b5}}, {{0x1}, {0x0}}}, {{{0x9f4b1777f5f1e, 0xe643ac62e82b8, 0x7b728399d6b4d, 0x346953b4e45df, 0x36770be67a13}, {0xaa5d1f4d4ddd5, 0xa6512be7d3bcf, 0x8c589e3e36988, 0xc401370c7984d, 0x2ebfbe492703}}, {{0x1}, {0x0}}}, {{{0xb65c8e43ab5d4, 0xefa3c24c7bffe, 0xd3ebac3e40619, 0x9c0baf0ba6b0d, 0x19c24e3baf00}, {0x1a21f778e28b3, 0x41f24a7edaed7, 0xfb25319d9ef03, 0xe20204439bf49, 0x2ae2342fb7fb}}, {{0x1}, {0x0}}}},
// {{{{0x7f8a5493adc2f, 0x1d19746738b8f, 0x6a20dfa164ac9, 0xde079020a050e, 0x21bc3d4c54f2}, {0x86243be8127ca, 0xed9ed6d9f6b0d, 0xb46ab784673b8, 0x951f42925ab41, 0x3dce3f2673f3}}, {{0x4}, {0x0}}}, {{{0x660197cbfa268, 0x84da6d8c9877e, 0xc3a9a6567502d, 0xc5cf911e829bf, 0x3685bfe39f71}, {0xb2d4411544c2c, 0x58cba12472939, 0xc15e1a0a0d716, 0xd2368557a2a0e, 0x37413e1e688f}}, {{0x1}, {0x0}}}, {{{0xd0548d8df9824, 0xd461c624976da, 0xb431338d30908, 0x92788ea2e3433, 0x53453ab4d978}, {0x2eccf309997db, 0x4285fa50de43a, 0x13bbc79e69f6c, 0x29f7ef72a8711, 0x346ce916b2ff}}, {{0x1}, {0x0}}}, {{{0xcaf34160fca19, 0x95737c974ac27, 0xd9a87a6b7f300, 0xaa781a6ff6de4, 0x50849c990615}, {0xb51d5226de67b, 0x65edcc08ba31a, 0x97c2f5c771669, 0x74cca7004b10a, 0x3cda79061aab}}, {{0x1}, {0x0}}}, {{{0xa5e4057368e36, 0x9ea948b581f1e, 0x827ddd35a1a25, 0xc19c2f2be2b4f, 0x4259360b27df}, {0xf7e8749f84851, 0xda7d2d124d6a1, 0xe0702cfc66e1a, 0x88031dcabb02b, 0x5054ce703f89}}, {{0x1}, {0x0}}}},
// {{{{0x6286782205516, 0xbf67207299834, 0xfcaf6270aebe7, 0xd10372ad9c315, 0x4eaaf9279442}, {0x42ad2285cebf2, 0x1d7b0a196031c, 0xb783aae838d0b, 0x431a20483e2e8, 0x3d59b8a84e21}}, {{0x4}, {0x0}}}, {{{0x11a572a6be8ec, 0x978d4d05ff53a, 0x6f45b3f58b4a9, 0x9a5a67bdcff3, 0x443cb55b07c}, {0xae6636e32f145, 0xd3ef8ac24f4ee, 0x75de880cd1cff, 0x10893d1375a4, 0x450ccbc46c48}}, {{0x1}, {0x0}}}, {{{0x65986baf8a566, 0x2a8d89d8ab3d9, 0xd64da8d48e1cd, 0xd9874f0af53ef, 0x293a97ff1082}, {0x590c3d152107c, 0x10b5d93949b11, 0xbb334825a6ba2, 0xf36221ecd5023, 0x2af07d5ad518}}, {{0x1}, {0x0}}}, {{{0xba489e5dd6e11, 0x10514e9f0f837, 0x84b25866caeb1, 0x12cff6fa69279, 0xfac6ce36b7a}, {0xb1dd944d54691, 0xefc51aa0c1bf, 0x9f4841802edfc, 0xa326fa7b5f015, 0x111f02b07179}}, {{0x1}, {0x0}}}, {{{0x6600f0329bf64, 0xa75645bb79f7, 0x9934a0509eb6b, 0x873a258aac729, 0x47000a3f885b}, {0x772148dd585bb, 0x3a9cc27bfd53c, 0xc6dfe526c61b9, 0xac8640bb0d332, 0x23ff1c2cc254}}, {{0x1}, {0x0}}}},
// {{{{0x7bab555354b7a, 0x4cabdd149ee31, 0xdc3dc9f3d0e59, 0x318acb475aa9e, 0x4c3514d4634}, {0xedefd4bd0a9af, 0x8743613fc900e, 0x57a6852c94441, 0xfc84d7a019500, 0xa06a9e44829}}, {{0x4}, {0x0}}}, {{{0x9e9d568876dc5, 0xd3194a4602fbf, 0x7800f830f4005, 0x813cfa39c30db, 0x1924f9fd3fae}, {0xee408fb2bbaa0, 0xcb5193762b66f, 0x14c3c3773cf55, 0x41f953827bffe, 0x2feb91059c0d}}, {{0x1}, {0x0}}}, {{{0x5ae43ccdf04d2, 0xaa76c6abeb1ec, 0xcbc98e775e1de, 0x1630c2f522bae, 0xc3b313a323e}, {0x11c92ae6ea68e, 0x44096a41f70b0, 0xd710c62604229, 0x4e36283f5f974, 0x231219098d31}}, {{0x1}, {0x0}}}, {{{0x943180d0d35bf, 0x7faeefba3a353, 0x9054efd33a6b4, 0x2dd4e90f9d0bb, 0x2233cd5ed21f}, {0x25b3931771086, 0xf64ef220918d, 0x30ee2dfdafbb0, 0x48ce5543ef43e, 0x17ad554dec45}}, {{0x1}, {0x0}}}, {{{0x2f1b72ff1024a, 0x470676208916b, 0xefdbda600780f, 0x74ececae5672f, 0x1cf65f0b3f17}, {0x76c78b31eda9d, 0xff3abf1ee4f9, 0x663e76a742d82, 0x12483f7ff7942, 0x8a158adf688}}, {{0x1}, {0x0}}}},
// {{{{0xc94354d74fc16, 0x669c63503361d, 0x2caca7577aa39, 0xdbf4057d5b85c, 0x4e7f8d769367}, {0xff37e7501cea9, 0xf81b3d9d24cb1, 0x3f021e87bb8e9, 0x3702f3cd1ae23, 0x4d3147be85f7}}, {{0x4}, {0x0}}}, {{{0x111df5f4c307b, 0xf731c32713a66, 0x88210d2e6692, 0x41f9e117f670c, 0x863592afac9}, {0xca7747b3a767e, 0xfcfad9b8b0938, 0xba394b402383d, 0x91ba1a51f0d1a, 0x2177fc074d59}}, {{0x1}, {0x0}}}, {{{0xd102eb07a1f6, 0xc93e1e39c0a51, 0x2ccff0c71e256, 0x6c91c2c9aecc3, 0xb7f98632153}, {0x9a7132bac4a44, 0x899886407066d, 0xbcb9e3aad320a, 0x8541e77ba680c, 0x16c808319d67}}, {{0x1}, {0x0}}}, {{{0xe3dcd5d5cd42d, 0x4279adf5613cc, 0x999d3c225326, 0xadc9bc883b249, 0x12baebafb166}, {0xe8d738f2d1a21, 0x6d35fe1d14180, 0x839e435c32bd3, 0xa50d9efed3c61, 0x2e90bec00e23}}, {{0x1}, {0x0}}}, {{{0xe3c90684f2339, 0x8844888d45a26, 0xf036a1cbbdee5, 0xe76eb27362f36, 0x1bbdd0515fa5}, {0x26563c95b1c6f, 0x34bad59f1f7cd, 0x31d06c0feb965, 0x59b83922a670a, 0x19aaef5fcb8f}}, {{0x1}, {0x0}}}},
// {{{{0xbc014d5d51919, 0x2f704a5dffcfd, 0x4f66a98edb1f, 0x15ebed124d028, 0x3a24fdcc9bb1}, {0x1a0fb8956a316, 0xca23499d55703, 0x467a3bbf1d42c, 0xaabb4d1b06019, 0x2a9bb5997f4b}}, {{0x4}, {0x0}}}, {{{0xaeb7c2ab44624, 0x7dd58f3801d43, 0xd06b4f6a51f59, 0xd3bdc0d607882, 0x41f41bc19fb0}, {0x63f7aac72e48b, 0xa8e575b143df5, 0xb499acfd5882c, 0x68a197409a527, 0x132fcbe2b1b}}, {{0x1}, {0x0}}}, {{{0xd1430ba1dd785, 0x1aac6677fa467, 0x602ea979c038d, 0x2801cfc111787, 0x51674bb5523c}, {0x80c4d7c1d63e4, 0xe1318f6d31de5, 0xc847f97a01888, 0x4dcc490a98768, 0x3a5ef277cf99}}, {{0x1}, {0x0}}}, {{{0x6a4ac478d67b8, 0x2e1cc202ba620, 0x66d83ac6544f, 0x9a863753e0e1b, 0x7d481849bac}, {0x5fd06f0cc1672, 0x9b8a74d374873, 0xd11bd166e5d7, 0x6408ee1f77873, 0x26a57934839c}}, {{0x1}, {0x0}}}, {{{0xad0066425ce3f, 0xcfc4a5f703f92, 0xa5545d179233e, 0x972238ac21eab, 0x8f3149a4ca3}, {0x8fd4acd2d0f15, 0x3d85b73048977, 0xfeed5b29ea470, 0x73f70efb34ad7, 0x309665fdbd2b}}, {{0x1}, {0x0}}}},
// {{{{0x7027d7fe9e81c, 0x5c48b9089eb52, 0xcf56e5a830540, 0xb9bd3301ca669, 0x8ca60fe497f}, {0x8a33a5bbd605c, 0xe04b39d589829, 0x67ce82bbd35c, 0xab17dfd31c27f, 0xeba0f522a0b}}, {{0x4}, {0x0}}}, {{{0x636af47d844c6, 0x566a013f9486, 0xa4eb8de4e3c1c, 0x19deff363d7b8, 0x3d60459b3dfe}, {0xf7ef0adc693e5, 0x20151d0622212, 0x8c45aaad1df9a, 0x73b535064dbb4, 0xb2be936dab1}}, {{0x1}, {0x0}}}, {{{0x10f73b1fc10cd, 0x5c74d01fba132, 0xfcfea80c6f38c, 0x326b8019fd936, 0x4c50726ca82d}, {0x6750b53896ee1, 0xb3b80534a3ac0, 0x1977bfe93a187, 0x52a0d7f746a86, 0x192806777548}}, {{0x1}, {0x0}}}, {{{0xc2660b91a70d2, 0x9ee0bfd6b4092, 0x1c4feb62ffed7, 0xd5bcb02528583, 0x38944ebf5d55}, {0xf4154914a0e77, 0x649af1bfd94f5, 0xede072191a734, 0xffb1a5b734a14, 0x2abef5640cda}}, {{0x1}, {0x0}}}, {{{0xa72438d5b625c, 0xa7cb5eb7e1463, 0xf5e174105d565, 0xaeafaf322dbb, 0x41ad6e26425}, {0x7b81a637f28bd, 0x410a540312ff9, 0xe2d997648a115, 0x3937833da4de6, 0x1734f9fb1bbb}}, {{0x1}, {0x0}}}}};

// const fp2_t alpha_sq = {{0xdb2fa85c839cf, 0xc734a1ed2141c, 0xed812e4329990, 0xb153e47e1b04, 0x4c10b0ecd8cb}, {0x9856830366352, 0x7a633281f8629, 0xafd62d9309606, 0x2335f1ba2fe9d, 0x10c19d44b38f}};

// const fp_t ell_mont[] = {{0xbc90408b20522, 0xfb13bc780e895, 0x82d27b8fae38d, 0x83c825b2b3447, 0x326000f7c811},
//                          {0x61b5766340894, 0x7604edd329d40, 0x7bf30d9f77548, 0x5a7c14eb86c69, 0x1c142bb32447},
//                          {0x6daac3b60c06, 0xf0f61f2e451eb, 0x75139faf40702, 0x313004245a48b, 0x5c8566e807d},
//                          {0x15fd87b2912e1, 0x29827453e14d6, 0xf4766446d336e, 0xa0522173774e3, 0x2d026cc3f6a9},
//                          {0x6f6aceda020a0, 0x57f12c2fb4116, 0x661a4cfdf834e, 0xbcdc1d343b580, 0x27a4d8902541},
//                          {0x7e8daa513277b, 0x907d815550401, 0xe57d11958afb9, 0x2bfe3a83585d8, 0x4edeeee59b6e},
//                          {0x6dfd4bd9931d1, 0x1511566a2201, 0xd0dec7c4e64e9, 0xb01a082dd2e3e, 0xbfb6f17b00f},
//                          {0xd7faf178a353a, 0xbeec393123041, 0x5720fa4caff99, 0x488836441c675, 0x49815ab1ca06},
//                          {0xd68d6e783466b, 0x684c22681112c, 0xc1e575139e134, 0x3bc6213db3f33, 0x2dd7f13954d4},
//                          {0x7bb2a450549dd, 0xe33d53c32c5d7, 0xbb060723672ee, 0x127a107687755, 0x178c1bf4b10a},
//                          {0x8ad57fc7850b8, 0x1bc9a8e8c88c2, 0x3a68cbbaf9f5a, 0x819c2dc5a47ae, 0x3ec6324a2736},
//                          {0x3f1d9116d5b05, 0xcf472f6980058, 0xb2ec226255d7f, 0xc7723a4d95028, 0x4fb4735af998},
//                          {0x971d553dd79f5, 0xa715d07c40d83, 0x8f5485e068efa, 0xd73a2107f0983, 0x2ead75aeb2fe},
//                          {0xf08a9c65487b4, 0xd5848858139c3, 0xf86e978deda, 0xf3c41cc8b4a20, 0x294fe17ae196},
//                          {0xffad77dc78e8f, 0xe10dd7dafcae, 0x805b332f20b46, 0x62e63a17d1a78, 0x5089f7d057c3},
//                          {0xef1d1964d98e5, 0x7ee4718f01aae, 0x6bbce95e7c075, 0xe70207c24c2de, 0xda678026c64},
//                          {0x488a608c4a6a4, 0xad53296ad46ef, 0xdd60d215a1055, 0x38c03831037a, 0x848e3ce9afd},
//                          {0xbf54d52cb7cc, 0x995d051128170, 0xd546ed548fae6, 0xb8842d5a1dc4d, 0x40713b34e38b},
//                          {0xb11a832aebb3e, 0x144e366c4361a, 0xce677f6458ca1, 0x8f381c92f146f, 0x2a2565f03fc1},
//                          {0xc03d5ea21c219, 0x4cda8b91df905, 0x4dca43fbeb90c, 0xfe5a39e20e4c8, 0x515f7c45b5ed},
//                          {0x183d22c91e109, 0x24a92ca4a0631, 0x2a32a779fea87, 0xe22209c69e23, 0x30587e996f54},
//                          {0xcc8534186eb56, 0xd826b32557dc6, 0xa2b5fe215a8ac, 0x53f82d245a69d, 0x4146bfaa41b6},
//                          {0x703ce6f01fff9, 0xfc77cdb76135c, 0x69b0af811c01, 0x1dea0756c577e, 0xf5180ed28ba},
//                          {0xda3a8c8f30362, 0xba12f181e219c, 0x8cdd3d7fdb6b2, 0xb658356d0efb4, 0x4cd76c8742b0},
//                          {0xc9aa2e1790db8, 0x2ae6859333f9c, 0x783ef3af36be2, 0x3a7403178981a, 0x9f3ecb95752},
//                          {0x7df23f66e1805, 0xde640c13eb732, 0xf0c24a5692a07, 0x804a0f9f7a094, 0x1ae22dca29b4},
//                          {0x8d151ade11ee0, 0x16f0613987a1d, 0x70250eee25673, 0xef6c2cee970ed, 0x421c441f9fe0},
//                          {0x9aca7354d36ec, 0xf8dc9f9611df3, 0x5a4c4e4ca6478, 0x51cc35374ba04, 0x4dacf0fca0db},
//                          {0xb35a1e4178966, 0xf74eebd02775, 0x423c297843ba, 0xfd081bbbe3daf, 0x2d7b77c5b86b},
//                          {0xb1ec9b4109a97, 0xb8d4d7f3f0860, 0x6ee83d5e72554, 0xf04606b57b66d, 0x11d20e4d4339},
//                          {0x727c8206ace21, 0xf79e8608204b7, 0x3c574e2b3d31a, 0x8bba067fb80bd, 0x12a792c2a164},
//                          {0xbb22bd8ab1653, 0xa473a5aefc980, 0xed96f6569c528, 0x770610ac4ad05, 0x16b6977f52df},
//                          {0xca459901e1d2e, 0xdcfffad498c6b, 0x6cf9baee2f193, 0xe6282dfb67d5e, 0x3df0add4c90b},
//                          {0xc76a930103f90, 0x2fbfcd4274e41, 0x4282b07c0b4c9, 0xcca403ee96edb, 0x69ddae3dea7},
//                          {0x2e8d329f3655b, 0x401ac37ad1e58, 0x9e4dd891b12af, 0x4b8e07f80f88e, 0xcd0f38d0e3a},
//                          {0x988ad83e468c4, 0xfdb5e74552c98, 0x24900b197ad5f, 0xe3fc360e590c5, 0x4a56df272830},
//                          {0x87fa79c6a731a, 0x6e897b56a4a98, 0xff1c148d628f, 0x681803b8d392b, 0x7735f593cd2},
//                          {0x3c428b15f7d67, 0x220701d75c22e, 0x887517f0320b5, 0xadee1040c41a5, 0x1861a06a0f34},
//                          {0x57ad3c037ad7f, 0xe5df7e90709da, 0x5cc396ad33cc0, 0x72ae20d22d3d3, 0x2f82fa241129},
//                          {0xfcd271db9b0f1, 0x60d0afeb8be84, 0x55e428bcfce7b, 0x4962100b00bf5, 0x193724df6d5f},
//                          {0x91a4751eda2e, 0xec1cd77f04346, 0xaacfe2e26be1b, 0x9f00034d4cdca, 0x91e6843f927},
//                          {0x71aa69f08eec8, 0x5317e48073271, 0x9bd6903123a67, 0x2aac1c5d2debf, 0x2afaea659dec},
//                          {0x415d2c2d6292d, 0xca6de7ba3f1b3, 0xe8a8659581498, 0x3542397687967, 0x530a85307243},
//                          {0x30cccdb5c3383, 0x3b417bcb90fb3, 0xd40a1bc4dc9c8, 0xb95e0721021cd, 0x1027056286e4},
//                          {0x995cf0546481d, 0xa23c88ccffede, 0xc510c91394613, 0x450a2030e32c2, 0x320387842ba9},
//                          {0x3e82262c84b8f, 0x1d2dba281b389, 0xbe315b235d7ce, 0x1bbe0f69b6ae4, 0x1bb7b23f87df},
//                          {0xf2ca377bd55dc, 0xd0ab40a8d2b1e, 0x36b4b1cab95f3, 0x61941bf1a735f, 0x2ca5f3505a41},
//                          {0x5b5a5a1a76a76, 0x37a64daa41a4a, 0x27bb5f197123f, 0xed40350188454, 0x4e827571ff05},
//                          {0x1bea40e019e00, 0x766ffbbe716a1, 0xf52a6fe63c005, 0x88b434cbc4ea3, 0x4f57f9e75d30}};

// const fp2_t Ms_mont = {{0xf2e1ed2aa1789, 0xf6a8f0fab8475, 0x6ea27ffd34f72, 0xaff9f9113be4e, 0x2b0037f3d00c}, {0, 0, 0, 0}};
// const fp2_t Mt_mont = {{0x22052f428c771, 0xcffa714f4b2b4, 0xdf11c5b582382, 0x2dcdf00a600f3, 0x4247e04c2bd7}, {0, 0, 0, 0}};
// const fp2_t MsxMt_mont = {{0x2dc228534e648, 0x989d2fc0010f2, 0x788ec00c12c36, 0xe22314355801, 0x1cd12ad0b0e6}, {0, 0, 0, 0}};

// const digit_t mscalar_s[] = {0xb47c9c79d89c2f29, 0xc8f99be655e653f, 0xc3f9c63ab3d9ab5a,
//                              0x3c29897df2deba63, 0x5985333f771f7715, 0x4153421391488e73,
//                              0x69632dd4a64a3e21, 0xc881bbbfd25fd25c, 0x15c66b5bdb182f7b,
//                              0xcdcbb9f18cc36a0b, 0xed8093ea9b75461e, 0x74223c949080fd3,
//                              0xf5f58b96b5c0aecf, 0x62b350c8857ddad2, 0x173a0c1db68032a,
//                              0x90d9ef5eac4018f9, 0xc4f50b8a5c368cf9, 0x3516f71f580073,
//                              0x52fc8a1fe11d198b, 0xe35c1852657952a2, 0x4d38ad44dd17e,
//                              0x3616f548b75fbc81, 0x5a7cbc6494dc7be0, 0x705270642a51,
//                              0x38bdd703d3c1ab9b, 0xf749eef7d1f0a834, 0x5e9643b2aa5,
//                              0x3a1e83c883a43f1d, 0x905120a699d2cfa9, 0x41ccb4a8cf,
//                              0xbb6227dac6bf0b01, 0xea7e623ab5db21a4, 0x244da1779,
//                              0xc3fae882eda310df, 0x49a0fae91ea3f8d3, 0x12bcb66f,
//                              0xe58f7c4e1ec589c7, 0xaa6195bac24eed17, 0x74fe34,
//                              0xf96e7fea00b732d5, 0x33971b4bc8fbe1ca, 0x2b884,
//                              0xdf2decef3109583b, 0xcaa12c27bd77bdfe, 0xed1,
//                              0xe2664cec5cb2e4af, 0x94ba9691a7cd2090, 0x47,
//                              0x561e493233c8d459, 0x2184305a838f93f, 0x1,
//                              0x24bc552e00a7ced7, 0x3445b952288d28c,
//                              0x1f493575c25236ed, 0xa139c03801b36,
//                              0x6da7a76ea062b8b5, 0x1cfc000a1190,
//                              0xf692e20e561b68d5, 0x4c7eae4a13,
//                              0x348c239ba5e38bbf, 0xb704057a,
//                              0x67d2502e0d43d95b, 0x1add5c6,
//                              0x1e5ebf02ac4b5c8b, 0x3cdc9,
//                              0xf86a3ffe10f2e559, 0x76e,
//                              0xe3ea5a29186a6a51, 0xd,
//                              0x17dd7e0725f667cd,
//                              0x2875d093bf833b,
//                              0x41f92cf76fb7,
//                              0x679d54c1dd,
//                              0x9ed595db,
//                              0xe0a64f,
//                              0xfb23,
//                              0x10d,
//                              0x1};

// const digit_t mscalar_t[] = {0xb43c57c62392efeb, 0x4860eb5fc7527,
//                              0xa130b9de7a902c3b, 0x441efba5704d,
//                              0x1231bfd5949c1cdf, 0x1d752e8cbaf,
//                              0x711f3751b8c18fad, 0x7fd10f66f,
//                              0x7b8f33447cbbd431, 0x218692ba,
//                              0x879354d32be43c7b, 0x80191d,
//                              0x839c51d1cbff9ea3, 0x1c138,
//                              0x9e739b6c885b3e67, 0x472,
//                              0xdf72e3f308c1201, 0xb,
//                              0x16487f5b34caad7f,
//                              0x290a1ff72bd15d,
//                              0x3cbaa2b93971,
//                              0x56da4b674b,
//                              0x7468f075,
//                              0x9a68b5,
//                              0xc6a3,
//                              0xf1,
//                              0x1};

// const int mscalar_s_bits[] = {192, 191, 189, 187, 185, 182, 179, 175, 171, 167, 162, 157, 151, 146, 140, 135, 129, 122, 116, 109, 103, 96, 89, 82, 75, 68, 61, 54, 47, 39, 32, 24, 16, 9, 1};
// const int mscalar_s_limbs[] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1};
// const int mscalar_t_bits[] = {115, 111, 105, 99, 94, 88, 81, 75, 68, 61, 54, 46, 39, 31, 24, 16, 8, 1};
// const int mscalar_t_limbs[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1};

const size_t degree_s_num = 35;
// const digit_t degree_s_value[] = {3, 3, 3, 3, 5, 7, 11, 11, 19, 23, 29, 31, 41, 43, 47, 53, 71, 79, 83, 89, 97, 107, 109, 113, 131, 137, 149, 151, 157, 163, 167, 181, 229, 239, 269};
// const int degree_s_bits[] = {2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9};

const size_t degree_t_num = 18;
// const digit_t degree_t_value[] = {13, 17, 37, 59, 61, 67, 73, 101, 103, 127, 139, 173, 179, 191, 193, 199, 211, 241};
// const int degree_t_bits[] = {4, 5, 6, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8};

const int strategy_s[] = {17, 8, 4, 3, 2, 1, 2, 1, 3, 2, 1, 4, 3, 2, 1, 3, 2, 1, 10, 4, 2, 1, 3, 2, 1, 6, 3, 2, 1, 4, 1, 3, 2, 1};
const int strategy_t[] = {10, 4, 3, 2, 1, 3, 2, 1, 6, 3, 2, 1, 4, 1, 3, 2, 1};
const size_t stack_volume = 6;

// const digit_t strascalar_s[] = {0x24bc552e00a7ced7, 0x3445b952288d28c, 0x1423fb24672b, 0xce8d, 0x69, 0x9, 0x3, 0x23, 0x7, 0x12c7, 0x1b5, 0x17, 0xd53233, 0xd57d, 0x6e3, 0x2b, 0x4893d, 0x15e9, 0x4f, 0xf86a3ffe10f2e559, 0x76e, 0xa4a63dd, 0x21b9, 0x61, 0x189ed7, 0x39d3, 0x83, 0x41f92cf76fb7, 0x35e637, 0x5c9b, 0x9d, 0x9ed595db, 0xa7, 0xe0a64f, 0xfb23, 0x10d};
// const int strascalar_s_bits[] = {122, 45, 16, 7, 4, 2, 6, 3, 13, 9, 5, 24, 16, 11, 6, 19, 13, 7, 75, 28, 14, 7, 21, 14, 8, 47, 22, 15, 8, 32, 8, 24, 16, 9};
// const int strascalar_s_limbs[] = {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// const digit_t strascalar_t[] = {0x9e739b6c885b3e67, 0x472, 0x1cbcccb, 0x90f7, 0x887, 0x3b, 0x789a7, 0x1ccd, 0x65, 0x3cbaa2b93971, 0x2e9991, 0x5def, 0xad, 0x7468f075, 0xbf, 0x9a68b5, 0xc6a3, 0xf1};
// const int strascalar_t_bits[] = {75, 25, 16, 12, 6, 19, 13, 7, 46, 22, 15, 8, 31, 8, 24, 16, 8};
// const int strascalar_t_limbs[] = {2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};