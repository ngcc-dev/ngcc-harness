/*
 * BiT-512 NTT Comprehensive Diagnostic Test
 *
 * Strategy: test every component in isolation and in cross-combination
 * against a known-good scalar reference to pinpoint exactly which
 * function (fwd NTT, inv NTT, pointwise mul, pointwise acc, mont_lift)
 * produces wrong results.
 *
 * Tests:
 *   T1  – forward NTT  (AVX2 vs ref)
 *   T2  – inverse NTT  (AVX2 vs ref)
 *   T3  – pointwise mul (AVX2 vs ref)
 *   T4  – pointwise acc (AVX2 vs ref)
 *   T5  – montgomery lift (AVX2 vs ref)
 *   T6  – round-trip ref:   ref_forward → ref_inverse = identity
 *   T7  – round-trip avx:   avx_forward → avx_inverse = identity
 *   T8  – cross: ref_fwd → avx_inv  (isolates avx_inv correctness)
 *   T9  – cross: avx_fwd → ref_inv  (isolates avx_fwd correctness)
 *   T10 – full mul round-trip:  fwd→pw_mul→inv→mont_lift
 *         (ref vs avx on all components)
 *   T11 – edge cases: zero input, const input, delta input
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "../params.h"
#include "../reduce.h"
#include "../align.h"

/* ================================================================
   Scalar reference NTT  (identical to ref/BiT-512/ntt.c)
   ================================================================ */
#define REF_INVERSE_Q    16257025
#define REF_INVERSE_N    32760
#define REF_MONT_R2      441335

static const int32_t ref_M[BIT_N] = {
      253888,  154188,  437836,  400801,  227836,  153807,  172184,  407938,
      354816,   23639,   58651,  310853,   16415,  487307,  437983,  161373,
      226876,  305466,  194263,  364592,  494934,  318132,   84038,  180577,
      496583,  332353,  331677,  331978,  396380,  421520,  127936,  492592,
      485707,  236350,  371569,  356762,   59221,  497158,  221858,  353674,
      160695,  517691,  262526,  476949,  277229,   12261,  119390,   44756,
      187990,  326582,  103485,  435986,  382360,  393041,   71074,  141331,
      257083,  226537,  193666,   73637,  184111,   30667,  421908,  433155,
      435584,  476964,  143125,  351025,  483859,  125144,  271018,  195292,
      300717,  151099,  134063,  446377,  148671,   37337,  500051,  480243,
      424777,  417203,  456157,  434195,  204369,  500958,  353726,  114555,
       75019,  404061,  245414,  332214,  140713,  192368,  382882,  508900,
      459060,  338094,  268615,  244916,  201655,  289223,  187477,  419004,
       54874,  294684,  180407,  316707,  379773,  172622,  422639,   10434,
      352948,  143177,  142795,  516950,  513328,  133542,   63061,  410715,
      485912,  472189,  429169,  100503,  117471,   24421,  437329,  111884,
      247489,  321402,   98951,  414235,   58976,  202614,   13455,  304681,
      421273,  476575,   42562,  432040,  464796,  433303,  357775,    9725,
      460085,  476903,   36422,    4007,  492905,    6310,  224446,  250440,
      257460,  454711,  223468,  505730,  227796,   95102,   21381,  102686,
       61432,  165750,  221401,    8090,   48244,  317949,  284122,  179909,
       68675,  456922,   48859,  505273,  266609,  291970,  396151,  150458,
      149113,   35786,  319739,  107888,    5903,  145230,  237584,  154908,
      416231,  489560,   46898,   33153,  389707,  316653,  498697,  183706,
      175065,  149501,  468480,  378007,  300369,  420655,  226598,  287611,
       24360,  378221,  285059,  189567,  501862,   81978,   89429,   94576,
      111142,  424417,  232301,  139316,  115260,  355829,  437786,  457468,
      304288,  255082,   58978,   75501,  351349,   72289,  213708,  226921,
      113395,   24601,  104081,   10208,  121060,   24773,  456115,  502603,
       48717,  166822,  467774,  512298,   51299,  444927,  292431,  344942,
      440982,  206164,  492569,   33381,  177974,  192254,    4387,  261179,
      227747,  348309,   83739,  196926,  272983,  413186,   55567,  206338,
      260807,  229946,  136195,  194099,   62322,  301502,   65465,   37438,
       28268,  131480,   88973,  465725,   13664,  286294,  223275,   27406,
      459709,  445269,  387530,   47719,  246502,  368411,  240320,    8786,
      133410,   73235,   69123,   74032,  225690,  255490,  482720,  469267,
      255015,   52550,   43988,  314053,  129377,  201547,  509215,  144444,
      220278,  375695,    2466,  367957,   91902,  277734,    2885,    7530,
      238562,  419811,  217022,  278845,  491308,  328502,   16793,  391828,
      295632,   35952,  405445,  395595,  467368,  306587,   14683,  286249,
      337961,  451156,   42607,  433059,  200681,  290287,   78746,  476897,
       10077,  158829,  446116,   68104,    8625,  368706,  329955,   15548,
      426127,  447773,  213780,  332590,  156587,  470907,  224790,  235110,
       50024,   69260,  492443,  238605,  397303,  280583,   57398,   97522,
      397654,   80454,   67207,  123124,   84673,  137157,  306237,  449494,
      155262,  151907,  106250,  267401,  395542,  101988,  323858,  235840,
      383195,  122953,  293001,   11054,  416873,  261341,  100408,  146672,
      351682,  495984,  129646,   11121,  178974,   99300,  133111,   89584,
       37199,  299038,  454156,  423563,  331039,  305971,  241904,  252732,
      133943,  270262,  218883,  344106,  377140,  274837,  418713,  360806,
      360796,  216852,  495497,  169043,  332658,  146115,  506489,  175194,
       97851,  100330,   75767,  201362,  360651,  329167,  404005,  362987,
      324432,  167919,  263268,  135396,  315007,  185692,   38411,  257124,
      131479,  295385,   84815,  475608,  489012,   79993,  412059,  348848,
      171373,  193056,  230439,  397708,  177498,  273954,  238584,   61954,
      466485,  506229,  236024,  466378,  459047,  384039,  518715,  301767,
      335391,  450759,  107133,   67759,  472748,  139418,  270256,  247396,
      385189,  318384,  345761,  454385,  387142,  258570,  387001,  116659,
      343628,  510123,  378820,  139482,  504112,  132928,  379058,   98632,
      465806,  224977,  134055,  434636,  253424,  513596,   41023,  189113,
       43370,   57302,  297599,  126733,  422426,  283047,  188900,  231590,
       25468,  183674,  456816,  425998,   93501,  348622,  452165,  297559,
      315391,  229067,  133618,  378501,  448506,   64425,  195979,   21975,
      505022,   37936,   38427,  280606,   53762,  223264,   45589,  257828,
      375349,  442358,  181926,  270185,  192107,   61454,  448253,  278333,
      192540,  371815,  481102,  305470,  317649,  291758,  349369,   27453,
      330079,  457630,  263983,  209386,  511254,  340894,  228888,  397266,
      337115,  119883,  312407,   11218,   36419,  454773,  374559,  193272,
      174916,  516042,  413927,  229310,  334157,  330337,  318665,  222286,
      476026,  398604,  430036,  397572,  404183,  494176,  506057,   61373,
      443776,   15116,  440231,  339436,  274059,  171675,    2643,  302606,
      468866,  294269,  486335,  377729,    6400,   29326,  199602,  462371,
      306178,  297880,   31766,  453447,   76377,  446372,  449352,  395698,
       80267,  303262,  159395,  430454,   71334,  262817,   72778,   41199,
      413666,  431477,  455395,  486299,  168377,  217692,  327521,  214748,
      313490,  105101,  132364,  488823,  352411,  200376,   29974,  424428,
      478170,  424034,  398066,  425019,  137946,  488066,  315930,   44755,
      510245,  225567,  313325,  448160,  370596,   34173,  508529,   47991,
       53526,  137001,  144218,  329536,   22119,   15359,  302462,  306212,
      199640,  127996,  458695,  387628,  393880,  133712,  326319,  141290,
       40053,  456144,  367926,  147110,  228483,  388095,   49472,  299517,
       84955,  420979,   42906,  416710,  121961,  241693,  328645,   43683,
       81597,   44383,  101713,  176223,  436561,   25134,  354200,  159968,
      321482,  390114,  195648,  511553,  111157,  251359,  223828,  513882,
      517076,  172174,  304149,  506251,  357704,  100596,  438729,   85787,
      479645,   52840,  431876,  497037,  239871,  195136,  483318,  436569,
      185542,  375187,  341854,  377473,  371902,  260264,  152288,  338879,
       24971,  299578,  494798,  118535,  447098,   78781,  193682,   97119,
      166409,  450564,  294754,  177908,  416388,  134760,   27573,  346939,
      400408,  350894,  384259,  123927,  486583,  221507,   84823,  487349,
      460343,  205309,  514918,  516342,  210961,  291875,  340835,  377518,
      414521,  450838,  492627,  508648,   68756,  250679,  211182,   31003,
      468163,  497987,   85807,  370913,  504465,  325879,  110301,   35458,
      404600,  260862,  166469,  278525,  167896,  357078,  217747,  237463,
       46231,   29609,  276774,   59757,  461423,  229765,  425055,  435058,
      402138,  158872,  274028,  321251,  394525,  495113,  482173,  121645,
      202416,   40579,  312486,  452281,  497538,  238380,  485820,   77071,
      506328,  264027,  202379,   51301,   82291,  152577,  195209,   62338,
      361053,  268911,   72890,  205573,  253432,    5144,  487338,  354202,
      268417,  344471,  345899,    6676,  270780,  236142,  502339,  196818,
      445295,  488787,   67712,   18961,  299673,  439574,  411668,  490272,
      242298,  180735,   48734,  386844,   17675,  515839,  246444,  413337,
      310840,  507487,  415477,   33212,  195742,  259365,  302337,  187783,
       63310,  451033,  305006,  398499,  125116,  255337,  453865,  451653,
       24827,   88240,  264023,  268091,  507682,   40371,  443256,  292337,
      235850,   80963,  292400,  494518,  418523,  472102,  208764,  253685,
      284667,  134451,  356892,  209367,  485549,  394610,    9984,   87364,
      318628,  233008,  281878,  267813,  213713,  169235,  416260,  467097,
      273808,  258470,  109745,  519941,  343535,  438658,  327314,  106022,
      274997,  377873,  443825,  282102,  161192,  401787,  224544,  264219,
      273263,  303928,   70802,  262330,   23736,  502809,  325420,  317450,
      352520,  295323,  453917,  267873,  404401,  163877,  313557,  268456,
      243636,  453790,  437367,  297702,  321737,   49093,   51607,  116666,
      436738,  479976,  150180,  106187,  113944,  245110,  106085,   90267,
      330126,  331536,  480191,  203922,  346982,  100559,  356952,   37328,
      222573,  427664,  266676,  455325,  426827,  174628,  511964,  472570,
       85694,    9999,  110983,  386137,  128943,  214839,   95456,  161695,
      131955,  213685,  370811,  154640,  230949,  235859,  425167,   79239,
      497619,   32137,  127950,  122994,   18311,  148766,   95266,  272991,
       28772,  350970,  116396,  462472,  321813,  420729,  390152,  384529,
      285816,  195149,   19976,  186469,   88838,  462668,  207737,  241989,
      407173,  330686,  204287,  510661,  328061,  226976,  432857,  311174,
      439317,  168461,  392686,   71995,   98187,   73259,   94049,  372591,
      345273,  258377,  143205,  468435,  476715,   44291,  179561,  505627,
      117517,  221980,  272627,  150808,  467615,  474018,  464713,  246466,
       41758,   32359,   98419,  413748,    2445,  402161,  179480,  191677,
      385870,  472523,  169164,  397351,  187329,  461892,   95387,  125453,
      119124,  304609,  127836,   85733,  340607,  302996,  316626,   25836
};

static const int32_t ref_Mn[BIT_N] = {
      253888,  366005,  119392,   82357,  112255,  348009,  366386,  292357,
      358820,   82210,   32886,  503778,  209340,  461542,  496554,  165377,
       27601,  392257,   98673,  123813,  188215,  188516,  187840,   23610,
      339616,  436155,  202061,   25259,  155601,  325930,  214727,  293317,
       87038,   98285,  489526,  336082,  446556,  326527,  293656,  263110,
      378862,  449119,  127152,  137833,   84207,  416708,  193611,  332203,
      475437,  400803,  507932,  242964,   43244,  257667,    2502,  359498,
      166519,  298335,   23035,  460972,  163431,  148624,  283843,   34486,
      408309,   82864,  495772,  402722,  419690,   91024,   48004,   34281,
      109478,  457132,  386651,    6865,    3243,  377398,  377016,  167245,
      509759,   97554,  347571,  140420,  203486,  339786,  225509,  465319,
      101189,  332716,  230970,  318538,  275277,  251578,  182099,   61133,
       11293,  137311,  327825,  379480,  187979,  274779,  116132,  445174,
      405638,  166467,   19235,  315824,   85998,   64036,  102990,   95416,
       39950,   20142,  482856,  371522,   73816,  386130,  369094,  219476,
      324901,  249175,  395049,   36334,  169168,  377068,   43229,   84609,
      313855,  464626,  107007,  247210,  323267,  436454,  171884,  292446,
      259014,  515806,  327939,  342219,  486812,   27624,  314029,   79211,
      175251,  227762,   75266,  468894,    7895,   52419,  353371,  471476,
       17590,   64078,  495420,  399133,  509985,  416112,  495592,  406798,
      293272,  306485,  447904,  168844,  444692,  461215,  265111,  215905,
       62725,   82407,  164364,  404933,  380877,  287892,   95776,  409051,
      425617,  430764,  438215,   18331,  330626,  235134,  141972,  495833,
      232582,  293595,   99538,  219824,  142186,   51713,  370692,  345128,
      336487,   21496,  203540,  130486,  487040,  473295,   30633,  103962,
      365285,  282609,  374963,  514290,  412305,  200454,  484407,  371080,
      369735,  124042,  228223,  253584,   14920,  471334,   63271,  451518,
      340284,  236071,  202244,  471949,  512103,  298792,  354443,  458761,
      417507,  498812,  425091,  292397,   14463,  296725,   65482,  262733,
      269753,  295747,  513883,   27288,  516186,  483771,   43290,   60108,
      510468,  162418,   86890,   55397,   88153,  477631,   43618,   98920,
      215512,  506738,  317579,  461217,  105958,  421242,  198791,  272704,
      262365,  474604,  296929,  466431,  239587,  481766,  482257,   15171,
      498218,  324214,  455768,   71687,  141692,  386575,  291126,  204802,
      222634,   68028,  171571,  426692,   94195,   63377,  336519,  494725,
      288603,  331293,  237146,   97767,  393460,  222594,  462891,  476823,
      331080,  479170,    6597,  266769,   85557,  386138,  295216,   54387,
      421561,  141135,  387265,   16081,  380711,  141373,   10070,  176565,
      403534,  133192,  261623,  133051,   65808,  174432,  201809,  135004,
      272797,  249937,  380775,   47445,  452434,  413060,   69434,  184802,
      218426,    1478,  136154,   61146,   53815,  284169,   13964,   53708,
      458239,  281609,  246239,  342695,  122485,  289754,  327137,  348820,
      171345,  108134,  440200,   31181,   44585,  435378,  224808,  388714,
      263069,  481782,  334501,  205186,  384797,  256925,  352274,  195761,
      157206,  116188,  191026,  159542,  318831,  444426,  419863,  422342,
      344999,   13704,  374078,  187535,  351150,   24696,  303341,  159397,
      159387,  101480,  245356,  143053,  176087,  301310,  249931,  386250,
      267461,  278289,  214222,  189154,   96630,   66037,  221155,  482994,
      430609,  387082,  420893,  341219,  509072,  390547,   24209,  168511,
      373521,  419785,  258852,  103320,  509139,  227192,  397240,  136998,
      284353,  196335,  418205,  124651,  252792,  413943,  368286,  364931,
       70699,  213956,  383036,  435520,  397069,  452986,  439739,  122539,
      422671,  462795,  239610,  122890,  281588,   27750,  450933,  470169,
      285083,  295403,   49286,  363606,  187603,  306413,   72420,   94066,
      504645,  190238,  151487,  511568,  452089,   74077,  361364,  510116,
       43296,  441447,  229906,  319512,   87134,  477586,   69037,  182232,
      233944,  505510,  213606,   52825,  124598,  114748,  484241,  224561,
      128365,  503400,  191691,   28885,  241348,  303171,  100382,  281631,
      512663,  517308,  242459,  428291,  152236,  517727,  144498,  299915,
      375749,   10978,  318646,  390816,  206140,  476205,  467643,  265178,
       50926,   37473,  264703,  294503,  446161,  451070,  446958,  386783,
      511407,  279873,  151782,  273691,  472474,  132663,   74924,   60484,
      492787,  296918,  233899,  506529,   54468,  431220,  388713,  491925,
      482755,  454728,  218691,  457871,  326094,  383998,  290247,  259386,
      494357,  203567,  217197,  179586,  434460,  392357,  215584,  401069,
      394740,  424806,   58301,  332864,  122842,  351029,   47670,  134323,
      328516,  340713,  118032,  517748,  106445,  421774,  487834,  478435,
      273727,   55480,   46175,   52578,  369385,  247566,  298213,  402676,
       14566,  340632,  475902,   43478,   51758,  376988,  261816,  174920,
      147602,  426144,  446934,  422006,  448198,  127507,  351732,   80876,
      209019,   87336,  293217,  192132,    9532,  315906,  189507,  113020,
      278204,  312456,   57525,  431355,  333724,  500217,  325044,  234377,
      135664,  130041,   99464,  198380,   57721,  403797,  169223,  491421,
      247202,  424927,  371427,  501882,  397199,  392243,  488056,   22574,
      440954,   95026,  284334,  289244,  365553,  149382,  306508,  388238,
      358498,  424737,  305354,  391250,  134056,  409210,  510194,  434499,
       47623,    8229,  345565,   93366,   64868,  253517,   92529,  297620,
      482865,  163241,  419634,  173211,  316271,   40002,  188657,  190067,
      429926,  414108,  275083,  406249,  414006,  370013,   40217,   83455,
      403527,  468586,  471100,  198456,  222491,   82826,   66403,  276557,
      251737,  206636,  356316,  115792,  252320,   66276,  224870,  167673,
      202743,  194773,   17384,  496457,  257863,  449391,  216265,  246930,
      255974,  295649,  118406,  359001,  238091,   76368,  142320,  245196,
      414171,  192879,   81535,  176658,     252,  410448,  261723,  246385,
       53096,  103933,  350958,  306480,  252380,  238315,  287185,  201565,
      432829,  510209,  125583,   34644,  310826,  163301,  385742,  235526,
      266508,  311429,   48091,  101670,   25675,  227793,  439230,  284343,
      227856,   76937,  479822,   12511,  252102,  256170,  431953,  495366,
       68540,   66328,  264856,  395077,  121694,  215187,   69160,  456883,
      332410,  217856,  260828,  324451,  486981,  104716,   12706,  209353,
      106856,  273749,    4354,  502518,  133349,  471459,  339458,  277895,
       29921,  108525,   80619,  220520,  501232,  452481,   31406,   74898,
      323375,   17854,  284051,  249413,  513517,  174294,  175722,  251776,
      165991,   32855,  515049,  266761,  314620,  447303,  251282,  159140,
      457855,  324984,  367616,  437902,  468892,  317814,  256166,   13865,
      443122,   34373,  281813,   22655,   67912,  207707,  479614,  317777,
      398548,   38020,   25080,  125668,  198942,  246165,  361321,  118055,
       85135,   95138,  290428,   58770,  460436,  243419,  490584,  473962,
      282730,  302446,  163115,  352297,  241668,  353724,  259331,  115593,
      484735,  409892,  194314,   15728,  149280,  434386,   22206,   52030,
      489190,  309011,  269514,  451437,   11545,   27566,   69355,  105672,
      142675,  179358,  228318,  309232,    3851,    5275,  314884,   59850,
       32844,  435370,  298686,   33610,  396266,  135934,  169299,  119785,
      173254,  492620,  385433,  103805,  342285,  225439,   69629,  353784,
      423074,  326511,  441412,   73095,  401658,   25395,  220615,  495222,
      181314,  367905,  259929,  148291,  142720,  178339,  145006,  334651,
       83624,   36875,  325057,  280322,   23156,   88317,  467353,   40548,
      434406,   81464,  419597,  162489,   13942,  216044,  348019,    3117,
        6311,  296365,  268834,  409036,    8640,  324545,  130079,  198711,
      360225,  165993,  495059,   83632,  343970,  418480,  475810,  438596,
      476510,  191548,  278500,  398232,  103483,  477287,   99214,  435238,
      220676,  470721,  132098,  291710,  373083,  152267,   64049,  480140,
      378903,  193874,  386481,  126313,  132565,   61498,  392197,  320553,
      213981,  217731,  504834,  498074,  190657,  375975,  383192,  466667,
      472202,   11664,  486020,  149597,   72033,  206868,  294626,    9948,
      475438,  204263,   32127,  382247,   95174,  122127,   96159,   42023,
       95765,  490219,  319817,  167782,   31370,  387829,  415092,  206703,
      305445,  192672,  302501,  351816,   33894,   64798,   88716,  106527,
      478994,  447415,  257376,  448859,   89739,  360798,  216931,  439926,
      124495,   70841,   73821,  443816,   66746,  488427,  222313,  214015,
       57822,  320591,  490867,  513793,  142464,   33858,  225924,   51327,
      217587,  517550,  348518,  246134,  180757,   79962,  505077,   76417,
      458820,   14136,   26017,  116010,  122621,   90157,  121589,   44167,
      297907,  201528,  189856,  186036,  290883,  106266,    4151,  345277,
      326921,  145634,   65420,  483774,  508975,  207786,  400310,  183078,
      122927,  291305,  179299,    8939,  310807,  256210,   62563,  190114,
      492740,  170824,  228435,  202544,  214723,   39091,  148378,  327653,
      241860,   71940,  458739,  328086,  250008,  338267,   77835,  144844
};

/* ---- scalar Montgomery multiplication ---- */
static inline int32_t ref_mont_mul(int64_t a, int64_t b) {
    int64_t prod = a * b;
    uint32_t m   = (uint32_t)prod * (uint32_t)REF_INVERSE_Q;
    int64_t  t   = (int64_t)m * BIT_Q;
    int32_t  r   = (int32_t)((prod - t) >> 32);
    r = r + ((r >> 31) & BIT_Q);
    return r;
}

/* ---- scalar NTT forward (CT, lazy reduction) ---- */
static void ref_ntt_forward(int32_t a[BIT_N]) {
    int t = BIT_N;
    for (int m = 1; m < BIT_N; m <<= 1) {
        t >>= 1;
        for (int i = 0; i < m; i++) {
            int s = (i * t) << 1;
            int e = s + t;
            int32_t S = ref_M[m + i];
            for (int j = s; j < e; j++) {
                int64_t U = a[j];
                int64_t V = a[j + t];
                V = ref_mont_mul(V, S);
                a[j]     = reduce_modq(U + V);
                a[j + t] = reduce_modq(U - V + BIT_Q);
            }
        }
    }
}

/* ---- scalar NTT inverse (GS, lazy reduction) ---- */
static void ref_ntt_inverse(int32_t a[BIT_N]) {
    int t = 1;
    for (int m = BIT_N; m > 1; m >>= 1) {
        int s = 0;
        int h = m >> 1;
        for (int i = 0; i < h; i++) {
            int e = s + t;
            int32_t S = ref_Mn[h + i];
            for (int j = s; j < e; j++) {
                int64_t U = a[j];
                int64_t V = a[j + t];
                a[j]     = reduce_modq(U + V);
                a[j + t] = ref_mont_mul(reduce_modq(U - V + BIT_Q), S);
            }
            s += 2 * t;
        }
        t <<= 1;
    }
    for (int i = 0; i < BIT_N; i++)
        a[i] = ref_mont_mul(a[i], REF_INVERSE_N);
}

static void ref_pointwise_mul(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]) {
    for (int i = 0; i < BIT_N; i++)
        r[i] = ref_mont_mul(a[i], b[i]);
}

static void ref_pointwise_acc(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]) {
    for (int i = 0; i < BIT_N; i++) {
        int32_t tmp = ref_mont_mul(a[i], b[i]);
        int32_t val = r[i] + reduce_modq(tmp) - BIT_Q;
        val += (val >> 31) & BIT_Q;
        r[i] = val;
    }
}

static void ref_montgomery_lift(int32_t r[BIT_N]) {
    for (int i = 0; i < BIT_N; i++)
        r[i] = ref_mont_mul(r[i], REF_MONT_R2);
}

/* AVX2 externs */
extern void ntt_avx_forward(int32_t a[BIT_N]);
extern void ntt_avx_inverse(int32_t a[BIT_N]);
extern void ntt_pointwise_mul_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
extern void ntt_pointwise_acc_raw(int32_t r[BIT_N], const int32_t a[BIT_N], const int32_t b[BIT_N]);
extern void ntt_montgomery_lift(int32_t r[BIT_N]);

/* ================================================================
   Test utilities
   ================================================================ */
static int g_seed = 0x5A5A5A5A;

/* Deterministic pseudo-random in [0, q) — reproducible across runs */
static void fill_rand(int32_t a[BIT_N]) {
    for (int i = 0; i < BIT_N; i++) {
        g_seed = g_seed * 1103515245 + 12345;
        uint32_t v = (uint32_t)(g_seed >> 16);
        /* map to [0, q) with bias < 2^-14 since q ≈ 2^19, state 32-bit */
        a[i] = (int32_t)(v % BIT_Q);
    }
}

static void fill_zero(int32_t a[BIT_N]) {
    for (int i = 0; i < BIT_N; i++) a[i] = 0;
}

static void fill_constant(int32_t a[BIT_N], int32_t val) {
    for (int i = 0; i < BIT_N; i++) a[i] = (int32_t)reduce_to_unsigned(val);
}

static void fill_delta(int32_t a[BIT_N], int pos, int32_t val) {
    for (int i = 0; i < BIT_N; i++) a[i] = 0;
    a[pos] = (int32_t)reduce_to_unsigned(val);
}

/* Compare two arrays in [0,q), return first mismatch index or -1.
   Also collects basic statistics. */
static int cmp_detailed(const int32_t *a, const int32_t *b, int n,
                         int *total_mismatches, int *first_idx,
                         int32_t *first_exp, int32_t *first_got) {
    int first = -1;
    int count = 0;
    for (int i = 0; i < n; i++) {
        /* Barrett handles signed, >Q, and [0,Q) representations uniformly */
        uint32_t va = reduce_barrett((int64_t)a[i]);
        uint32_t vb = reduce_barrett((int64_t)b[i]);
        if (va != vb) {
            count++;
            if (first < 0) {
                first = i;
                *first_idx = i;
                *first_exp = (int32_t)va;
                *first_got = (int32_t)vb;
            }
        }
    }
    *total_mismatches = count;
    return first;
}

/* Dump first N mismatches for detailed analysis */
static void dump_mismatches(const int32_t *a, const int32_t *b, int n, int max_show) {
    int shown = 0;
    printf("  Mismatch details (idx : expected -> got):\n");
    for (int i = 0; i < n && shown < max_show; i++) {
        uint32_t va = reduce_barrett((int64_t)a[i]);
        uint32_t vb = reduce_barrett((int64_t)b[i]);
        if (va != vb) {
            printf("    [%4d]  %8d -> %8d  (diff=%d)\n", i, (int32_t)va, (int32_t)vb, (int32_t)(vb - va + BIT_Q) % BIT_Q);
            shown++;
        }
    }
    if (shown >= max_show) {
        /* Count remaining */
        int remaining = 0;
        for (int i = 0; i < n; i++) {
            uint32_t va = reduce_barrett((int64_t)a[i]);
            uint32_t vb = reduce_barrett((int64_t)b[i]);
            if (va != vb) remaining++;
        }
        if (remaining > shown)
            printf("    ... and %d more mismatches\n", remaining - shown);
    }
}

/* ================================================================
   Test macros
   ================================================================ */
#define NTESTS 50

static int test_count   = 0;
static int fail_count   = 0;
static int pass_count   = 0;

#define BEGIN_TEST(name) do { \
    printf("\n=== T%d: %-55s ===\n", ++test_count, name); \
    fflush(stdout); \
} while(0)

#define RUN_SUBTEST(label, call_ref, call_avx, inputs_equal) do { \
    int total_mis = 0, first_i = 0; \
    int32_t first_e = 0, first_g = 0; \
    int fails_this = 0; \
    for (int _t = 0; _t < NTESTS; _t++) { \
        int32_t ref_out[BIT_N] __attribute__((aligned(32))); \
        int32_t avx_out[BIT_N] __attribute__((aligned(32))); \
        int32_t in_a[BIT_N]   __attribute__((aligned(32))); \
        int32_t in_b[BIT_N]   __attribute__((aligned(32))); \
        fill_rand(in_a); fill_rand(in_b); \
        if (inputs_equal) { memcpy(ref_out, in_a, sizeof(in_a)); memcpy(avx_out, in_a, sizeof(in_a)); } \
        else { memcpy(ref_out, in_a, sizeof(in_a)); memcpy(avx_out, in_a, sizeof(in_a)); } \
        call_ref; call_avx; \
        if (cmp_detailed(ref_out, avx_out, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) { \
            if (fails_this == 0) { \
                printf("  " label " FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n", \
                       _t, total_mis, first_i, first_e, first_g); \
                dump_mismatches(ref_out, avx_out, BIT_N, 8); \
            } \
            fails_this++; \
        } \
    } \
    if (fails_this == 0) { printf("  " label " PASS (%d iterations)\n", NTESTS); pass_count++; } \
    else { printf("  " label " FAILED %d/%d iterations\n", fails_this, NTESTS); fail_count++; } \
} while(0)

/* ================================================================
   Main
   ================================================================ */
int main(void) {
    printf("BiT-512 NTT Diagnostic Suite\n");
    printf("Q=%d  N=%d  BarrettMultiplier=%d  BarrettShift=%d\n",
           BIT_Q, BIT_N, BIT_BARRETT_MULT, BIT_BARRETT_SHIFT);
    printf("Montgomery constants: INV_Q=%u  R2=%d  INV_N=%d\n",
           REF_INVERSE_Q, REF_MONT_R2, REF_INVERSE_N);
    printf("=============================================================\n");

    /* ---------- T1: forward NTT ---------- */
    BEGIN_TEST("Forward NTT (AVX2 vs scalar ref)");
    RUN_SUBTEST("fwd", ref_ntt_forward(ref_out), ntt_avx_forward(avx_out), 1);

    /* ---------- T2: inverse NTT ---------- */
    BEGIN_TEST("Inverse NTT (AVX2 vs scalar ref)");
    RUN_SUBTEST("inv", ref_ntt_inverse(ref_out), ntt_avx_inverse(avx_out), 1);

    /* ---------- T3: pointwise mul ---------- */
    BEGIN_TEST("Pointwise multiply (AVX2 vs scalar ref)");
    RUN_SUBTEST("pwmul",
        ref_pointwise_mul(ref_out, in_a, in_b),
        ntt_pointwise_mul_raw(avx_out, in_a, in_b), 0);

    /* ---------- T4: pointwise acc ---------- */
    BEGIN_TEST("Pointwise accumulate (AVX2 vs scalar ref)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t ref_out[BIT_N] __attribute__((aligned(32)));
            int32_t avx_out[BIT_N] __attribute__((aligned(32)));
            int32_t in_a[BIT_N]   __attribute__((aligned(32)));
            int32_t in_b[BIT_N]   __attribute__((aligned(32)));
            int32_t ref_base[BIT_N] __attribute__((aligned(32)));
            fill_rand(in_a); fill_rand(in_b); fill_rand(ref_base);
            memcpy(ref_out, ref_base, sizeof(ref_base));
            memcpy(avx_out, ref_base, sizeof(ref_base));
            ref_pointwise_acc(ref_out, in_a, in_b);
            ntt_pointwise_acc_raw(avx_out, in_a, in_b);
            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(ref_out, avx_out, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  pwacc FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n",
                           _t, total_mis, first_i, first_e, first_g);
                    dump_mismatches(ref_out, avx_out, BIT_N, 8);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  pwacc PASS (%d iterations)\n", NTESTS); pass_count++; }
        else { printf("  pwacc FAILED %d/%d iterations\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T5: montgomery lift ---------- */
    BEGIN_TEST("Montgomery lift (AVX2 vs scalar ref)");
    RUN_SUBTEST("lift", ref_montgomery_lift(ref_out), ntt_montgomery_lift(avx_out), 1);

    /* ---------- T6: round-trip ref ---------- */
    BEGIN_TEST("Round-trip: ref FWD → ref INV (should recover input)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t orig[BIT_N] __attribute__((aligned(32)));
            int32_t work[BIT_N] __attribute__((aligned(32)));
            fill_rand(orig);
            /* The scalar ref does: FWD then INV.  After FWD, values are in
               Montgomery domain and range [0,2Q). After INV, values are
               scaled by N.  The scalar reference multiplies by INV_N at the end
               to cancel this.  So FWD→INV should give back the original
               (modulo Montgomery domain transitions handled by INV_N). */
            /* Actually the ref NTT uses lazy reduction and the INV multiplies
               by INV_N at the end.  The composition should be:
               INV(FWD(a)) = a * N * INV_N = a (mod Q).  Let's verify this. */
            memcpy(work, orig, sizeof(orig));
            ref_ntt_forward(work);
            ref_ntt_inverse(work);
            /* Check recovery */
            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(orig, work, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  REF round-trip FAIL at iter %d: %d mismatches — REF IS BROKEN!\n",
                           _t, total_mis);
                    dump_mismatches(orig, work, BIT_N, 10);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  REF round-trip PASS (%d iters) — scalar ref is self-consistent\n", NTESTS); pass_count++; }
        else { printf("  REF round-trip FAILED %d/%d — scalar reference has a bug!\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T7: round-trip avx ---------- */
    BEGIN_TEST("Round-trip: AVX FWD → AVX INV (should recover input)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t orig[BIT_N] __attribute__((aligned(32)));
            int32_t work[BIT_N] __attribute__((aligned(32)));
            fill_rand(orig);
            memcpy(work, orig, sizeof(orig));
            ntt_avx_forward(work);
            ntt_avx_inverse(work);
            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(orig, work, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  AVX round-trip FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n",
                           _t, total_mis, first_i, first_e, first_g);
                    dump_mismatches(orig, work, BIT_N, 10);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  AVX round-trip PASS (%d iters) — AVX FWD+INV self-consistent\n", NTESTS); pass_count++; }
        else { printf("  AVX round-trip FAILED %d/%d — FWD, INV, or both are buggy\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T8: cross ref_fwd → avx_inv (isolates AVX INV) ---------- */
    BEGIN_TEST("Cross: ref FWD → AVX INV  (isolates AVX inverse NTT)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t orig[BIT_N] __attribute__((aligned(32)));
            int32_t avx_work[BIT_N] __attribute__((aligned(32)));
            int32_t ref_work[BIT_N] __attribute__((aligned(32)));
            fill_rand(orig);
            memcpy(avx_work, orig, sizeof(orig));
            memcpy(ref_work, orig, sizeof(orig));
            /* Both go through ref FWD */
            ref_ntt_forward(avx_work);
            ref_ntt_forward(ref_work);
            /* One does AVX INV, the other ref INV */
            ntt_avx_inverse(avx_work);
            ref_ntt_inverse(ref_work);
            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(ref_work, avx_work, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  CROSS inv FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n",
                           _t, total_mis, first_i, first_e, first_g);
                    dump_mismatches(ref_work, avx_work, BIT_N, 8);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  CROSS inv PASS — AVX inverse NTT matches ref\n"); pass_count++; }
        else { printf("  CROSS inv FAILED %d/%d — ** AVX INVERSE NTT IS BUGGY **\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T9: cross avx_fwd → ref_inv (isolates AVX FWD) ---------- */
    BEGIN_TEST("Cross: AVX FWD → ref INV  (isolates AVX forward NTT)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t orig[BIT_N] __attribute__((aligned(32)));
            int32_t avx_work[BIT_N] __attribute__((aligned(32)));
            int32_t ref_work[BIT_N] __attribute__((aligned(32)));
            fill_rand(orig);
            memcpy(avx_work, orig, sizeof(orig));
            memcpy(ref_work, orig, sizeof(orig));
            ntt_avx_forward(avx_work);
            ref_ntt_forward(ref_work);
            /* Both go through ref INV */
            ref_ntt_inverse(avx_work);
            ref_ntt_inverse(ref_work);
            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(ref_work, avx_work, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  CROSS fwd FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n",
                           _t, total_mis, first_i, first_e, first_g);
                    dump_mismatches(ref_work, avx_work, BIT_N, 8);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  CROSS fwd PASS — AVX forward NTT matches ref\n"); pass_count++; }
        else { printf("  CROSS fwd FAILED %d/%d — ** AVX FORWARD NTT IS BUGGY **\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T10: full mul round-trip ---------- */
    BEGIN_TEST("Full convolution: FWD(a),FWD(b) → pw_mul → INV → lift (ref chain vs avx chain)");
    {
        int fails_this = 0;
        for (int _t = 0; _t < NTESTS; _t++) {
            int32_t a[BIT_N] __attribute__((aligned(32)));
            int32_t b[BIT_N] __attribute__((aligned(32)));
            int32_t ref_a_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t ref_b_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t ref_c_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t ref_c[BIT_N]     __attribute__((aligned(32)));
            int32_t avx_a_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t avx_b_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t avx_c_ntt[BIT_N] __attribute__((aligned(32)));
            int32_t avx_c[BIT_N]     __attribute__((aligned(32)));

            fill_rand(a); fill_rand(b);

            /* Ref chain */
            memcpy(ref_a_ntt, a, sizeof(a));
            memcpy(ref_b_ntt, b, sizeof(b));
            ref_ntt_forward(ref_a_ntt);
            ref_ntt_forward(ref_b_ntt);
            ref_pointwise_mul(ref_c_ntt, ref_a_ntt, ref_b_ntt);
            memcpy(ref_c, ref_c_ntt, sizeof(ref_c_ntt));
            ref_ntt_inverse(ref_c);
            /* ref_montgomery_lift(ref_c);  — ref INV already exits Montgomery */

            /* AVX chain */
            memcpy(avx_a_ntt, a, sizeof(a));
            memcpy(avx_b_ntt, b, sizeof(b));
            ntt_avx_forward(avx_a_ntt);
            ntt_avx_forward(avx_b_ntt);
            ntt_pointwise_mul_raw(avx_c_ntt, avx_a_ntt, avx_b_ntt);
            memcpy(avx_c, avx_c_ntt, sizeof(avx_c_ntt));
            ntt_avx_inverse(avx_c);

            int total_mis = 0, first_i = 0;
            int32_t first_e = 0, first_g = 0;
            if (cmp_detailed(ref_c, avx_c, BIT_N, &total_mis, &first_i, &first_e, &first_g) >= 0) {
                if (fails_this == 0) {
                    printf("  full-mul FAIL at iter %d: %d mismatches, first at [%d] %d != %d\n",
                           _t, total_mis, first_i, first_e, first_g);
                    dump_mismatches(ref_c, avx_c, BIT_N, 8);
                }
                fails_this++;
            }
        }
        if (fails_this == 0) { printf("  full-mul PASS — complete NTT pipeline works\n"); pass_count++; }
        else { printf("  full-mul FAILED %d/%d — at least one NTT component is broken\n", fails_this, NTESTS); fail_count++; }
    }

    /* ---------- T11: edge cases ---------- */
    BEGIN_TEST("Edge cases: zero input, constant input, delta input");
    {
        int fails_this = 0;
        int32_t in[BIT_N]   __attribute__((aligned(32)));
        int32_t ref_o[BIT_N] __attribute__((aligned(32)));
        int32_t avx_o[BIT_N] __attribute__((aligned(32)));

        #define CHECK_EDGE(label) do { \
            int _tm = 0, _fi = 0; \
            int32_t _fe = 0, _fg = 0; \
            if (cmp_detailed(ref_o, avx_o, BIT_N, &_tm, &_fi, &_fe, &_fg) >= 0) { \
                if (fails_this == 0) \
                    printf("  " label " FAIL: %d mismatches, first at [%d] %d != %d\n", \
                           _tm, _fi, _fe, _fg); \
                fails_this++; \
            } \
        } while(0)

        /* Edge 1: all-zero input → fwd */
        fill_zero(in);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ntt_avx_forward(avx_o);
        CHECK_EDGE("zero→fwd");

        /* Edge 2: all-zero input → inv */
        fill_zero(in);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_inverse(ref_o); ntt_avx_inverse(avx_o);
        CHECK_EDGE("zero→inv");

        /* Edge 3: constant 1 → fwd */
        fill_constant(in, 1);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ntt_avx_forward(avx_o);
        CHECK_EDGE("const1→fwd");

        /* Edge 4: constant 1 → inv */
        fill_constant(in, 1);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_inverse(ref_o); ntt_avx_inverse(avx_o);
        CHECK_EDGE("const1→inv");

        /* Edge 5: constant Q-1 → fwd */
        fill_constant(in, (int32_t)(BIT_Q - 1));
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ntt_avx_forward(avx_o);
        CHECK_EDGE("maxval→fwd");

        /* Edge 6: constant Q/2 → fwd */
        fill_constant(in, (int32_t)(BIT_Q / 2));
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ntt_avx_forward(avx_o);
        CHECK_EDGE("midval→fwd");

        /* Edge 7: delta[0]=1 → fwd → inv round-trip */
        fill_delta(in, 0, 1);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ref_ntt_inverse(ref_o);
        ntt_avx_forward(avx_o); ntt_avx_inverse(avx_o);
        CHECK_EDGE("delta0-rt");

        /* Edge 8: delta[512]=1 → fwd → inv round-trip */
        fill_delta(in, 512, 1);
        memcpy(ref_o, in, sizeof(in)); memcpy(avx_o, in, sizeof(in));
        ref_ntt_forward(ref_o); ref_ntt_inverse(ref_o);
        ntt_avx_forward(avx_o); ntt_avx_inverse(avx_o);
        CHECK_EDGE("delta512-rt");

        /* Edge 9: pointwise mul with zero */
        {
            int32_t zero[BIT_N] __attribute__((aligned(32)));
            fill_zero(zero);
            fill_rand(in);
            ref_pointwise_mul(ref_o, in, zero);
            ntt_pointwise_mul_raw(avx_o, in, zero);
            CHECK_EDGE("pwmul-by-zero");
        }

        /* Edge 10: pointwise mul with one (as unsigned, 1 in Montgomery?) */
        /* Actually test with R=2^32 mod Q in Montgomery form */
        {
            int32_t one_mont = ref_mont_mul(1, REF_MONT_R2); /* 1*R mod Q */
            int32_t ones[BIT_N] __attribute__((aligned(32)));
            fill_constant(ones, one_mont);
            fill_rand(in);
            ref_pointwise_mul(ref_o, in, ones);
            ntt_pointwise_mul_raw(avx_o, in, ones);
            CHECK_EDGE("pwmul-by-1R");
        }

        #undef CHECK_EDGE

        if (fails_this == 0) { printf("  All edge cases PASS\n"); pass_count++; }
        else { printf("  Edge cases: %d failures\n", fails_this); fail_count++; }
    }

    /* ---------- summary ---------- */
    printf("\n=============================================================\n");
    printf("SUMMARY:  %d tests,  %d PASS,  %d FAIL\n",
           pass_count + fail_count, pass_count, fail_count);
    if (fail_count > 0) {
        printf("\n>>> Look at FAIL results above to identify which component(s) are buggy.\n");
        printf("    - T8 FAIL → bug in AVX inverse NTT\n");
        printf("    - T9 FAIL → bug in AVX forward NTT\n");
        printf("    - T3 FAIL → bug in pointwise multiply\n");
        printf("    - T4 FAIL → bug in pointwise accumulate\n");
        printf("    - T5 FAIL → bug in Montgomery lift\n");
        printf("    - T6 FAIL → bug in SCALAR REFERENCE (upstream issue)\n");
        printf("    - T10 FAIL with T8,T9,T3 all PASS → likely interaction issue\n");
    }
    printf("=============================================================\n");
    return (fail_count > 0) ? 1 : 0;
}
