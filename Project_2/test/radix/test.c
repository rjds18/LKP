#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/rbtree.h>
#include <linux/slab.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jae-Won");

struct mystruct {
  int data;
  struct rb_node rbnode;
};

/* 
-------------rb tree rules----------------
1) nodes are either BLACK or RED
2) Root / Leaf node is always BLACK 
3) Red node childrens are all BLACK. However, Black node children doesn't have to be RED
4) Black count of every nodes between Root and Leave nodes are equal

------------rb tree strategy--------------
< whenever new node is inserted, tree must be rebalanced to ensure the rule >
1) insert new node -> color it red
2) recolor and rotate nodes to fix violation.
----> new node = root
         recolor this node to black

----> new node.uncle = red 
         recolor new node's parent / grandparents / uncle

----> new node.uncle = black (triangle) > 
      new node's uncle is black while new node's parent is red (new node is
      an alternating child to grandparents -> parents)
         rotate new node's parent opposite direction of new node      
      
----> new node.uncle = black (line) \
      new node's uncle is black while new node's parent / grandparents form a line
         rotate new node's grandparent opposite direction of new node

----> determine direction of new node by using comparison function of > and <
*/

struct rb_root root = RB_ROOT;

int insert_rb_tree(struct rb_root *tree_root, struct mystruct *insertion)
{
  struct rb_node **new = &(tree_root->rb_node);
  struct rb_node *parent = NULL;
  
  while (*new){
    struct mystruct *this = container_of(*new, struct mystruct, rbnode);
    parent = *new;
    if (this->data > insertion->data)
      {
	new = &(*new)->rb_left;
      }
    else if (this->data < insertion->data)
      {
	new = &(*new)->rb_right;
      }
    else
      {
	return -1;
      }  
  }
  rb_link_node(&insertion->rbnode, parent, new);
  rb_insert_color(&insertion->rbnode, tree_root);

  return 0;
}
static void rb_tree_insert(int value)
{
  struct mystruct *target = kmalloc(sizeof(*target), GFP_KERNEL);
  target->data = value;
  insert_rb_tree(&root, target);
}

struct mystruct rb_tree_view(struct rb_root *root)
{
  struct rb_node *iternode;
  for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
    {
      struct mystruct *temp = rb_entry(iternode, struct mystruct, rbnode);
      printk("key=%d\n", temp->data);
    }
}

static void rb_tree_remove(struct rb_root *root)
{
  struct rb_node *iternode;

  for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
    {
      struct mystruct *temp = rb_entry(iternode, struct mystruct, rbnode);
      rb_erase(&(temp->rbnode), root);
    }

}

static int __init test_init(void)
{
  rb_tree_insert(2);
  rb_tree_insert(1);
  rb_tree_insert(3);
  rb_tree_insert(5);
  rb_tree_insert(4);
  
  rb_tree_view(&root);

  rb_tree_remove(&root);

  rb_tree_insert(18);
  rb_tree_insert(17);
  rb_tree_insert(3);
  rb_tree_view(&root);
  
}

static void __exit test_exit(void)
{
  printk(KERN_INFO "Goodbye\n");
}

module_init(test_init);
module_exit(test_exit);
