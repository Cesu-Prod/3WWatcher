#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

typedef struct Node {
    int value;
    Node *next;
} Node;

typedef struct Sensor {
    int min;
    int max;
    int errors;
    bool active;
    Node *head_list;
    
    void Init_list();
    void Update(int value);
    int Average();
}Sensor;

void Sensor::Init_list() {
    int i;
    Node *initial_node = new Node();
    initial_node->value = 0;
    initial_node->next = nullptr;
    head_list = initial_node;
    for (i = 1; i <= 3; i++) {
        Node *node = new Node;
        node->value = 0;
        node->next = head_list;
        head_list = node;
    }
}

void Sensor::Update(int value) {
    Node *current;
    Node *node = new Node();
    node->value = value;
    node->next = head_list;
    head_list = node;
    current = head_list;
    while (current->next->next != nullptr) {
        current = current->next;
    }
    delete current->next;
    current->next = nullptr;
}

int Sensor::Average() {
    Node *current;
    int count = 0;
    int total = 0;
    current = head_list;
    while (current != nullptr) {
        if (current->value != 0) {
            total += current->value;
            count++;
        }
        current = current->next;
    }
    if (count != 0) {
        return total / count;
    } else {
        return 0;
    }
}

static Sensor ssr_lum;
static Sensor ssr_hum;
static Sensor ssr_tmp;
static Sensor ssr_prs;




// Example of use.

void setup() {
    ssr_lum.Init_list();
    ssr_hum.Init_list();
    ssr_tmp.Init_list();
    ssr_prs.Init_list();
}

void loop() {
    static int lum = 0;
    static int hum = 0;
    static int tmp = 0;
    static int prs = 0;

    lum = random(0, 100);
    hum = random(0, 100);
    tmp = random(0, 100);
    prs = random(0, 100);

    ssr_lum.Update(lum);
    ssr_hum.Update(hum);
    ssr_tmp.Update(tmp);
    ssr_prs.Update(prs);
}