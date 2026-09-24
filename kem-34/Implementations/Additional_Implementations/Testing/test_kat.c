#include "api.h"
#include "drng.h"
#include "params.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NTESTS 100
// NOTE: This must match KAT_KEM.c
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static void
printstr (char *identifier, char *msg, unsigned long long len)
{
	printf ("%s", identifier);
	for (unsigned long long i = 0; i < len; i++)
		printf ("%02X", (unsigned char)msg[i]);
	printf ("\n");
}

// Convert a hex string to their byte counterparts.
static int
hex_to_bytes (char *output, const char *hex_str, size_t str_len)
{
	char byte[3] = { 0 };
	long byte_val;
	for (size_t i = 0; i < str_len; i++)
		{
			memcpy (byte, &hex_str[i * 2], 2);
			if (byte[0] == '\0' || byte[1] == '\0')
				return 1;
			byte_val = strtol (byte, NULL, 16);
			if (byte_val > 0xff)
				return 1;
			output[i] = (uint8_t)byte_val;
		}
	return 0;
}

struct KnownAnswerTest
{
	char seed[SEED_LEN_BYTES];
	char ss[KEM_SSBYTES];
	char pk[KEM_PUBLICKEYBYTES];
	char sk[KEM_SECRETKEYBYTES];
	char ct[KEM_CIPHERTEXTBYTES];
};

/*
 * verify_kat
 *
 * This verifies a single KAT. It takes as input a random seed and then
 * verifies the four main parts of the algorithm:
 * - PK: Public Key
 * - SK: Secret Key
 * - CT: Cipher Text
 * - SS: Shared Secret
 *
 * Args:
 * 	- seed: The random seed from which all the randomness in the scheme
 * _should_ be generated.
 *
 * Returns:
 * 	- 1 if any of the generated values don't match the KAT. 0 otherwise.
 */
static int
verify_kat (struct KnownAnswerTest *kat)
{
	int rtn = 0;
	unsigned long long pk_bytes = KEM_PUBLICKEYBYTES,
										 sk_bytes = KEM_SECRETKEYBYTES,
										 ct_bytes = KEM_CIPHERTEXTBYTES, ss_bytes = KEM_SSBYTES;
	unsigned char ss0[KEM_SSBYTES], ss1[KEM_SSBYTES], ct[KEM_CIPHERTEXTBYTES],
			pk[KEM_PUBLICKEYBYTES], sk[KEM_SECRETKEYBYTES];

	// init drng_algorithm using seed
	init_random_number (&drng_algorithm, (unsigned char *)kat->seed,
											SEED_LEN_BYTES);

	// TEST KEYGEN
	// Generate the new values
	rtn = kem_keygen (pk, &pk_bytes, sk, &sk_bytes);

	// Test generated values
	if (memcmp (pk, kat->pk, pk_bytes) != 0)
		{
			printf ("ERROR: KAT test failed, public keys don't match.\n");
			printstr ("\tKAT pk =", kat->pk, pk_bytes);
			printstr ("\tgen pk =", (char *)pk, pk_bytes);
			rtn = 1;
		}
	if (memcmp (sk, kat->sk, sk_bytes) != 0)
		{
			printf ("ERROR: KAT test failed, secret keys don't match.\n");
			printstr ("\tKAT sk =", kat->sk, sk_bytes);
			printstr ("\tgen sk =", (char *)sk, sk_bytes);
			rtn = 1;
		}

	// Bob derives a secret key and creates a response
	kem_enc (pk, pk_bytes, ss0, &ss_bytes, ct, &ct_bytes);

	if (memcmp (ss0, kat->ss, ss_bytes) != 0)
		{
			printf ("ERROR: KAT test failed, first shared secret doesn't match.\n");
			printstr ("\tKAT ss =", kat->ss, ss_bytes);
			printstr ("\tgen ss =", (char *)ss0, ss_bytes);
			rtn = 1;
		}
	if (memcmp (ct, kat->ct, ct_bytes) != 0)
		{
			printf ("ERROR: KAT test failed, ciphertext doesn't match.\n");
			printstr ("\tKAT ct =", kat->ct, ct_bytes);
			printstr ("\tgen ct =", (char *)ct, ct_bytes);
			rtn = 1;
		}

	// Alice uses Bobs response to get her shared key
	kem_dec (sk, KEM_SECRETKEYBYTES, ct, KEM_CIPHERTEXTBYTES, ss1, &ss_bytes);

	if (memcmp (ss1, kat->ss, ss_bytes) != 0)
		{
			printf ("ERROR: KAT test failed, second shared secret doesn't match.\n");
			printstr ("\tKAT ss =", kat->ss, ss_bytes);
			printstr ("\tgen ss =", (char *)ss1, ss_bytes);
			rtn = 1;
		}

	if (rtn != 0)
		{
			printstr ("\tSeed =", kat->seed, SEED_LEN_BYTES);
		}

	return rtn;
}

enum ReadState
{
	LINE_START,
	NAME,
	ASSIGNMENT,
	VALUE,
	SKIP_LINE,
};

static int
read_kat (struct KnownAnswerTest *kat, FILE *file)
{
	// FILE *file = fopen (fname, "r");
	char ch;
	char name[100] = "";
	char value[100000] = "";
	size_t name_i = 0, value_i = 0;
	enum ReadState state = LINE_START;

	char *val_ptr = kat->seed;
	size_t val_len = SEED_LEN_BYTES;

	while (fread (&ch, sizeof (char), 1, file) == 1)
		{
			switch (state)
				{
				case LINE_START:
					// Reset
					name_i = 0;
					value_i = 0;
					memset (name, 0, 100);
					memset (value, 0, 10000);
					if (ch == '\n')
						{
							// Finished processing 1 KAT
							return 0;
						}
					else
						{
							// start name
							state = NAME;
							// NOTE: Intentional fall-through to process ch
							[[fallthrough]];
						}
				case NAME:
					if (ch == ' ')
						{
							state = ASSIGNMENT;
							// Get the value we are assigning
							if (strcmp (name, "Seed") == 0)
								{
									val_ptr = kat->seed;
									val_len = SEED_LEN_BYTES;
								}
							else if (strcmp (name, "PK") == 0)
								{
									val_ptr = kat->pk;
									val_len = KEM_PUBLICKEYBYTES;
								}
							else if (strcmp (name, "SK") == 0)
								{
									val_ptr = kat->sk;
									val_len = KEM_SECRETKEYBYTES;
								}
							else if (strcmp (name, "CT") == 0)
								{
									val_ptr = kat->ct;
									val_len = KEM_CIPHERTEXTBYTES;
								}
							else if (strcmp (name, "SS") == 0)
								{
									val_ptr = kat->ss;
									val_len = KEM_SSBYTES;
								}
							else
								state = SKIP_LINE;
						}
					else
						{
							name[name_i++] = ch;
						}
					break;
				case ASSIGNMENT:
					if (ch != ' ' && ch != '=')
						{
							state = VALUE;
							// NOTE: Intentional fall-through to process ch
							[[fallthrough]];
						}
					else
						break;
				case VALUE:
					if (ch == ' ' || ch == '\n')
						{
							// Reset line
							state = LINE_START;
							// Process value
							// Convert hex to bytes
							hex_to_bytes (val_ptr, value, val_len);
						}
					else
						{
							value[value_i++] = ch;
						}
					break;
				case SKIP_LINE:
					if (ch == '\n')
						state = LINE_START;
					break;
				}
		}
	return 0;
}
/*
 * test_kat
 *
 * This function tests the correctness of the KATs by running the algorithm
 * with the given seeds. This ensures that any changes to the functioning
 * of the algorithm are detected.
 *
 * Args:
 * 	- fname: The name of the file containing the KAT output
 *
 * Output:
 *
 */
static int
test_kat (char *fname)
{
	// Open file and read into buffer
	FILE *file = fopen (fname, "r");
	// While we aren't at the end of the file
	while (!feof (file))
		{
			struct KnownAnswerTest kat;
			// Read a KAT
			if (!read_kat (&kat, file))
				{
					printstr ("INFO: Testing seed ", kat.seed, SEED_LEN_BYTES);
					if (verify_kat (&kat))
						return 1;
				}
		}
	return 0;
}

static void
print_usage (char *bin_name)
{
	printf ("USAGE:\n");
	printf ("\t%s <fname>\n\n", bin_name);
	printf ("ARGS:\n");
	printf (
			"fname: Path to the file containing the Known Answer Tests (KATs).\n");
}

int
main (int argc, char **argv)
{
	if (argc < 2)
		{
			printf ("ERROR: Please provide path to KAT file as first argument\n");
			print_usage (argv[0]);
			return 1;
		}

	if ((strncmp (argv[1], "-help", 2) == 0)
			|| (strncmp (argv[1], "--help", 3) == 0)
			|| (strncmp (argv[1], "-?", 2) == 0))
		{
			print_usage (argv[0]);
			return 0;
		}

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	return test_kat (argv[1]);
}
