#include "link.h"

#define SERIAL_DEBUG

// one link list init for program in setup
// link list of devices of [add, range[3]]
// range[0] is latest range, takes avera
struct MyLink *init_link()
{
#ifdef SERIAL_DEBUG
    Serial.println("init_link");
#endif
    struct MyLink *p = (struct MyLink *)malloc(sizeof(struct MyLink));
    p->next = NULL;
    p->anchor_addr = 0;
    p->tag_addr = 0;
    for (int i = 0; i < 100; i++) {
        p->payload[i] = 0; // Initialize each element to 0
    }
    p->range[0] = 0.0;
    p->range[1] = 0.0;
    p->range[2] = 0.0;

    return p;
}

// create a new {add, range[3]} and append to device
void add_link(struct MyLink *p, uint8_t addr, uint64_t tagaddr, uint8_t* payload)
{
#ifdef SERIAL_DEBUG
    Serial.println("add_link");
#endif
    struct MyLink *curr= find_link(p, tagaddr);
    if (curr != NULL) {
      Serial.println("link already exists!");
      Serial.println(curr->payload[1], HEX);
        return;
    }
    struct MyLink *temp = p;
    //Find struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
    }

    Serial.println("add_link:find struct MyLink end");
    //Create a anchor
    struct MyLink *a = (struct MyLink *)malloc(sizeof(struct MyLink));
    a->anchor_addr = addr;
    a->tag_addr = tagaddr;
    memcpy(a->payload, payload, 100);
    Serial.println("test");
    Serial.println(a->payload[0], HEX);
    a->range[0] = 0.0;
    a->range[1] = 0.0;
    a->range[2] = 0.0;
    a->next = NULL;

    //Add anchor to end of struct MyLink
    temp->next = a;

    return;
}

// find link by address, iterate down link list and returns struct
struct MyLink *find_link(struct MyLink *p, uint64_t tagaddr)
{
#ifdef SERIAL_DEBUG
    Serial.println("find_link");
#endif
    if (tagaddr == 0)
    {
        Serial.println("find_link:Input tagaddr is 0");
        return NULL;
    }

    if (p->next == NULL)
    {
        Serial.println("find_link:Link is empty");
        return NULL;
    }

    struct MyLink *temp = p;
    //Find target struct MyLink or struct MyLink end
    while (temp->next != NULL)
    {
        temp = temp->next;
        if (temp->tag_addr == tagaddr)
        {
            // Serial.println("find_link:Find addr");
            return temp;
        }
    }

    Serial.println("find_link:Can't find tagaddr");
    return NULL;
}


// finds existing struct link of address in linkedlist
// if exists, add new range to first position by taking average of it + past 2 range, push back the rest.
void fresh_link(struct MyLink *p, uint64_t tagaddr, double range)
{
#ifdef SERIAL_DEBUG
    Serial.println("fresh_link");
#endif
    struct MyLink *temp = find_link(p, tagaddr);
    if (temp != NULL)
    {
        temp->range[2] = temp->range[1];
        temp->range[1] = temp->range[0];
        temp->range[0] = (range + temp->range[1] + temp->range[2]) / 3;
        return;
    }
    else
    {
        Serial.println("fresh_link:Fresh fail");
        return;
    }
}

void print_link(struct MyLink *p)
{
#ifdef SERIAL_DEBUG
    Serial.println("print_link");
#endif
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        //Serial.println("Dev %d:%d m", temp->next->anchor_addr, temp->next->range);
        Serial.println(temp->next->anchor_addr, HEX);
        Serial.println(temp->next->tag_addr, HEX);
        Serial.println(temp->next->range[0]);
        for (int i = 0; i < 100; i++) {
          Serial.print(temp->next->payload[i]);
        }
        Serial.println();
        temp = temp->next;
    }

    return;
}

void delete_link(struct MyLink *p, uint8_t addr)
{
#ifdef SERIAL_DEBUG
    Serial.println("delete_link");
#endif
    if (addr == 0)
        return;

    struct MyLink *temp = p;
    while (temp->next != NULL)
    {
        if (temp->next->anchor_addr == addr)
        {
            struct MyLink *del = temp->next;
            temp->next = del->next;
            free(del);
            return;
        }
        temp = temp->next;
    }
    return;
}

void make_link_json(struct MyLink *p, String *s)
{
#ifdef SERIAL_DEBUG
    Serial.println("make_link_json");
#endif
    
    *s = "{\"links\":";
    struct MyLink *temp = p;

    while (temp->next != NULL)
    {
        temp = temp->next;
        char link_json[200];
        String str;
        char buff[3];
        for (int i = 0; i < 100; i++) {
          sprintf(buff, "%02X", temp->payload[i]);
          str += buff;
          // Serial.print(temp->payload[i]);          
        }
        // Serial.println();
        Serial.println(str.c_str());
        char str_cstr[201]; // Reserve space for the null terminator
        str.toCharArray(str_cstr, sizeof(str_cstr));
        sprintf(link_json, "{\"A\":\"%X\",\"R\":\"%.2f\",\"T\":\"%016llX\",\"Payload\":\"%s\"}", temp->anchor_addr, temp->range[0], temp->tag_addr, str_cstr);
        *s += link_json;
        if (temp->next != NULL)
        {
            *s += ",";
        }
    }
    *s += "}";
    Serial.println(*s);
}