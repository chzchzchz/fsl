#ifndef FSLINODE_H
#define FSLINODE_H

struct fsl_bridge_node;

#define fbn_by_ino(x)		(struct fsl_bridge_node*)((x)->i_private)
#define INO_TYPE_SHIFT		56
#define INO_TYPE_MASK		((0xffUL) << INO_TYPE_SHIFT)

#define ino_put_fbn(x,y)	do { x->i_private = y; } while (0)
#define ino_type(x)		(((x) & INO_TYPE_MASK) >> INO_TYPE_SHIFT)
#define typenum_to_dt(x)	((x == TYPENUM_INVALID) ? DT_REG : DT_DIR)

struct inode *fsl_inode_iget(struct super_block *sb, struct fsl_bridge_node* fbn);

#endif
