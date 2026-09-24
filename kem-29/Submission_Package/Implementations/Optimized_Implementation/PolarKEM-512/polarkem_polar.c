/**
 * @file polarkem_polar.c
 * @brief Portable word-sliced implementation of the Polar transform.
 */
#include "polarkem_polar.h"

#include <stddef.h>
#include <string.h>

/*
 * Exact BEC(epsilon=1/2) information set.  Reliability values are compared as
 * exact rationals, the first 512 ranks are selected, and selected indices are
 * stored in numerical order.  This table is part of the serialized profile.
 */
static const uint16_t polarkem_info_positions[POLARKEM_MU_BITS] = {
    511, 759, 763, 765, 766, 767, 863, 879, 887, 891, 893, 894, 895, 927, 942, 943,
    947, 949, 950, 951, 953, 954, 955, 956, 957, 958, 959, 967, 971, 973, 974, 975,
    979, 981, 982, 983, 985, 986, 987, 988, 989, 990, 991, 995, 997, 998, 999, 1001,
    1002, 1003, 1004, 1005, 1006, 1007, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016,
    1017, 1018, 1019, 1020, 1021, 1022, 1023, 1215, 1247, 1263, 1269, 1270, 1271, 1273,
    1274, 1275, 1276, 1277, 1278, 1279, 1343, 1371, 1373, 1374, 1375, 1383, 1387, 1389,
    1390, 1391, 1395, 1397, 1398, 1399, 1401, 1402, 1403, 1404, 1405, 1406, 1407, 1423,
    1431, 1435, 1437, 1438, 1439, 1447, 1451, 1453, 1454, 1455, 1459, 1461, 1462, 1463,
    1465, 1466, 1467, 1468, 1469, 1470, 1471, 1479, 1483, 1485, 1486, 1487, 1490, 1491,
    1492, 1493, 1494, 1495, 1496, 1497, 1498, 1499, 1500, 1501, 1502, 1503, 1505, 1506,
    1507, 1508, 1509, 1510, 1511, 1512, 1513, 1514, 1515, 1516, 1517, 1518, 1519, 1520,
    1521, 1522, 1523, 1524, 1525, 1526, 1527, 1528, 1529, 1530, 1531, 1532, 1533, 1534,
    1535, 1591, 1595, 1597, 1598, 1599, 1615, 1623, 1627, 1629, 1630, 1631, 1639, 1643,
    1645, 1646, 1647, 1650, 1651, 1652, 1653, 1654, 1655, 1656, 1657, 1658, 1659, 1660,
    1661, 1662, 1663, 1679, 1687, 1690, 1691, 1692, 1693, 1694, 1695, 1701, 1702, 1703,
    1705, 1706, 1707, 1708, 1709, 1710, 1711, 1713, 1714, 1715, 1716, 1717, 1718, 1719,
    1720, 1721, 1722, 1723, 1724, 1725, 1726, 1727, 1731, 1733, 1734, 1735, 1737, 1738,
    1739, 1740, 1741, 1742, 1743, 1745, 1746, 1747, 1748, 1749, 1750, 1751, 1752, 1753,
    1754, 1755, 1756, 1757, 1758, 1759, 1761, 1762, 1763, 1764, 1765, 1766, 1767, 1768,
    1769, 1770, 1771, 1772, 1773, 1774, 1775, 1776, 1777, 1778, 1779, 1780, 1781, 1782,
    1783, 1784, 1785, 1786, 1787, 1788, 1789, 1790, 1791, 1805, 1806, 1807, 1811, 1813,
    1814, 1815, 1817, 1818, 1819, 1820, 1821, 1822, 1823, 1827, 1829, 1830, 1831, 1833,
    1834, 1835, 1836, 1837, 1838, 1839, 1841, 1842, 1843, 1844, 1845, 1846, 1847, 1848,
    1849, 1850, 1851, 1852, 1853, 1854, 1855, 1859, 1861, 1862, 1863, 1865, 1866, 1867,
    1868, 1869, 1870, 1871, 1873, 1874, 1875, 1876, 1877, 1878, 1879, 1880, 1881, 1882,
    1883, 1884, 1885, 1886, 1887, 1889, 1890, 1891, 1892, 1893, 1894, 1895, 1896, 1897,
    1898, 1899, 1900, 1901, 1902, 1903, 1904, 1905, 1906, 1907, 1908, 1909, 1910, 1911,
    1912, 1913, 1914, 1915, 1916, 1917, 1918, 1919, 1923, 1925, 1926, 1927, 1929, 1930,
    1931, 1932, 1933, 1934, 1935, 1937, 1938, 1939, 1940, 1941, 1942, 1943, 1944, 1945,
    1946, 1947, 1948, 1949, 1950, 1951, 1953, 1954, 1955, 1956, 1957, 1958, 1959, 1960,
    1961, 1962, 1963, 1964, 1965, 1966, 1967, 1968, 1969, 1970, 1971, 1972, 1973, 1974,
    1975, 1976, 1977, 1978, 1979, 1980, 1981, 1982, 1983, 1985, 1986, 1987, 1988, 1989,
    1990, 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
    2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017,
    2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026, 2027, 2028, 2029, 2030, 2031,
    2032, 2033, 2034, 2035, 2036, 2037, 2038, 2039, 2040, 2041, 2042, 2043, 2044, 2045,
    2046, 2047
};

void polarkem_polar_transform(uint64_t words[POLARKEM_WORDS])
{
    size_t base;
    size_t j;
    size_t span_words;
    size_t w;

    /* Six butterfly layers contained entirely within each 64-bit word. */
    for (w = 0U; w < POLARKEM_WORDS; ++w) {
        uint64_t x = words[w];
        x ^= (x >> 1) & UINT64_C(0x5555555555555555);
        x ^= (x >> 2) & UINT64_C(0x3333333333333333);
        x ^= (x >> 4) & UINT64_C(0x0f0f0f0f0f0f0f0f);
        x ^= (x >> 8) & UINT64_C(0x00ff00ff00ff00ff);
        x ^= (x >> 16) & UINT64_C(0x0000ffff0000ffff);
        x ^= (x >> 32) & UINT64_C(0x00000000ffffffff);
        words[w] = x;
    }

    /* Remaining layers combine cache-adjacent groups of whole words. */
    for (span_words = 1U; span_words < POLARKEM_WORDS;
         span_words <<= 1U) {
        for (base = 0U; base < POLARKEM_WORDS;
             base += 2U * span_words) {
            for (j = 0U; j < span_words; ++j) {
                words[base + j] ^= words[base + span_words + j];
            }
        }
    }
}

void polarkem_polar_encode(uint64_t codeword[POLARKEM_WORDS],
                           const unsigned char mu[POLARKEM_MU_BYTES])
{
    size_t j;

    memset(codeword, 0, POLARKEM_WORDS * sizeof(codeword[0]));
    for (j = 0U; j < POLARKEM_MU_BITS; ++j) {
        const uint16_t position = polarkem_info_positions[j];
        const uint64_t bit = (uint64_t)((mu[j >> 3U] >> (j & 7U)) & 1U);
        codeword[position >> 6U] |= bit << (position & 63U);
    }
    polarkem_polar_transform(codeword);
}

void polarkem_polar_decode(unsigned char mu[POLARKEM_MU_BYTES],
                           const uint64_t codeword[POLARKEM_WORDS])
{
    uint64_t information[POLARKEM_WORDS];
    size_t j;

    memcpy(information, codeword, sizeof(information));
    polarkem_polar_transform(information);
    memset(mu, 0, POLARKEM_MU_BYTES);
    for (j = 0U; j < POLARKEM_MU_BITS; ++j) {
        const uint16_t position = polarkem_info_positions[j];
        const unsigned char bit = (unsigned char)(
            (information[position >> 6U] >> (position & 63U)) & UINT64_C(1));
        mu[j >> 3U] |= (unsigned char)(bit << (j & 7U));
    }
}

