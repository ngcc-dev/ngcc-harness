/* Phoenix address structure field offsets for SHAKE-based hash functions
 *
 * Each field is positioned at a specific byte offset within the 32-byte address.
 */

#if !defined( SHAKE_OFFSETS_H_ )
#define SHAKE_OFFSETS_H_

#define PH_OFFSET_LAYER       3   /* Merkle tree layer index */
#define PH_OFFSET_TREE        8   /* Tree identifier (8 bytes) */
#define PH_OFFSET_ADDR_TYPE   19  /* Hash operation type */
#define PH_OFFSET_KEYPAIR_ADDR 20 /* Key pair address (4 bytes) */
#define PH_OFFSET_CHAIN_ADDR  27  /* Winternitz chain position */
#define PH_OFFSET_HASH_ADDR   31  /* Hash step within chain */
#define PH_OFFSET_TREE_HEIGHT 27  /* Node height in TFORS/Merkle tree */
#define PH_OFFSET_TREE_INDEX  28  /* Node position in tree (4 bytes) */
#define PH_OFFSET_COUNTER     24  /* GWOTS counter value (4 bytes) */

#define SPX_SHAKE 1

#endif /* SHAKE_OFFSETS_H_ */