/*
 * Generated from 2,000 signatures (2,048,000 coefficients) with one-count
 * smoothing over the complete allowed high-part range.  With ell = 4, the
 * table has floor(4317 / 16) + 1 = 270 symbols; its size comes from the
 * rejection bound, not from the rANS normalization total.
 *
 * Frequencies sum to 2^16 = 65536.  This 16-bit normalization reduces the
 * quantization and tail-smoothing overhead seen with the earlier 2^12 model,
 * while remaining within the uint32_t state bounds used by codec.c.
 */
#if YUANYANG_REJECTION_BOUND != 4317u
#error The yuanyang-1024 rANS model assumes rejection bound 4317.
#endif
#if YUANYANG_RANS_SCALE_BITS != 16u
#error The yuanyang-1024 rANS model uses a 16-bit normalization total.
#endif
#if YUANYANG_SIG_LOW_BITS == 4u
#define YUANYANG_RANS_SYMBOLS 270u
static const uint16_t yuanyang_rans_freq[YUANYANG_RANS_SYMBOLS] = {
	9041u, 9052u, 8490u, 7700u, 6806u, 5779u, 4770u, 3776u,
	2939u, 2200u, 1590u, 1111u, 756u, 500u, 314u, 197u,
	115u, 70u, 37u, 22u, 12u, 7u, 3u, 3u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u
};
static const uint16_t yuanyang_rans_start[YUANYANG_RANS_SYMBOLS] = {
	0u, 9041u, 18093u, 26583u, 34283u, 41089u, 46868u, 51638u,
	55414u, 58353u, 60553u, 62143u, 63254u, 64010u, 64510u, 64824u,
	65021u, 65136u, 65206u, 65243u, 65265u, 65277u, 65284u, 65287u,
	65290u, 65291u, 65292u, 65293u, 65294u, 65295u, 65296u, 65297u,
	65298u, 65299u, 65300u, 65301u, 65302u, 65303u, 65304u, 65305u,
	65306u, 65307u, 65308u, 65309u, 65310u, 65311u, 65312u, 65313u,
	65314u, 65315u, 65316u, 65317u, 65318u, 65319u, 65320u, 65321u,
	65322u, 65323u, 65324u, 65325u, 65326u, 65327u, 65328u, 65329u,
	65330u, 65331u, 65332u, 65333u, 65334u, 65335u, 65336u, 65337u,
	65338u, 65339u, 65340u, 65341u, 65342u, 65343u, 65344u, 65345u,
	65346u, 65347u, 65348u, 65349u, 65350u, 65351u, 65352u, 65353u,
	65354u, 65355u, 65356u, 65357u, 65358u, 65359u, 65360u, 65361u,
	65362u, 65363u, 65364u, 65365u, 65366u, 65367u, 65368u, 65369u,
	65370u, 65371u, 65372u, 65373u, 65374u, 65375u, 65376u, 65377u,
	65378u, 65379u, 65380u, 65381u, 65382u, 65383u, 65384u, 65385u,
	65386u, 65387u, 65388u, 65389u, 65390u, 65391u, 65392u, 65393u,
	65394u, 65395u, 65396u, 65397u, 65398u, 65399u, 65400u, 65401u,
	65402u, 65403u, 65404u, 65405u, 65406u, 65407u, 65408u, 65409u,
	65410u, 65411u, 65412u, 65413u, 65414u, 65415u, 65416u, 65417u,
	65418u, 65419u, 65420u, 65421u, 65422u, 65423u, 65424u, 65425u,
	65426u, 65427u, 65428u, 65429u, 65430u, 65431u, 65432u, 65433u,
	65434u, 65435u, 65436u, 65437u, 65438u, 65439u, 65440u, 65441u,
	65442u, 65443u, 65444u, 65445u, 65446u, 65447u, 65448u, 65449u,
	65450u, 65451u, 65452u, 65453u, 65454u, 65455u, 65456u, 65457u,
	65458u, 65459u, 65460u, 65461u, 65462u, 65463u, 65464u, 65465u,
	65466u, 65467u, 65468u, 65469u, 65470u, 65471u, 65472u, 65473u,
	65474u, 65475u, 65476u, 65477u, 65478u, 65479u, 65480u, 65481u,
	65482u, 65483u, 65484u, 65485u, 65486u, 65487u, 65488u, 65489u,
	65490u, 65491u, 65492u, 65493u, 65494u, 65495u, 65496u, 65497u,
	65498u, 65499u, 65500u, 65501u, 65502u, 65503u, 65504u, 65505u,
	65506u, 65507u, 65508u, 65509u, 65510u, 65511u, 65512u, 65513u,
	65514u, 65515u, 65516u, 65517u, 65518u, 65519u, 65520u, 65521u,
	65522u, 65523u, 65524u, 65525u, 65526u, 65527u, 65528u, 65529u,
	65530u, 65531u, 65532u, 65533u, 65534u, 65535u
};
#else
#error The yuanyang-1024 rANS model is generated for ell=4.
#endif
