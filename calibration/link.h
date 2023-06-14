#include <Arduino.h>

struct MyLink
{
    uint8_t anchor_addr;
    double range[3];
    struct MyLink *next;
};

struct MyLink *init_link();
void add_link(struct MyLink *p, uint8_t addr);
struct MyLink *find_link(struct MyLink *p, uint8_t addr);
void fresh_link(struct MyLink *p, uint8_t addr, double range);
void print_link(struct MyLink *p);
void delete_link(struct MyLink *p, uint8_t addr);
void make_link_json(struct MyLink *p,String *s);