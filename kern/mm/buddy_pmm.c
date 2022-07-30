#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>
#include <stdio.h>
#define MAX_NUM 11
free_area_t free_area;
free_area_t core_array[MAX_NUM];
int tool_array[11] = {1,2,4,8,16,32,64,128,256,512,1024};
#define PAGE_BITS 14
// #define free_list_ (free_area.free_list)
// #define nr_free_ (free_area.nr_free)
static void
merge_page(struct Page *base, int index);
static void
buddy_init(void)
{
    for(int i=0;i< MAX_NUM;i++){
        list_init(&core_array[i].free_list);
        core_array[i].nr_free = 0;
    }
}

static void
add_page_into_list(struct Page *base, int index){
    uintptr_t current_pa = page2pa(base);
    list_entry_t * head = &(core_array[index].free_list);
    list_entry_t * temp = head->next;
    base->property = tool_array[index];
    SetPageProperty(base);
    if( temp == head){
        cprintf("in add_page_into_list function, the head of index %u is blank\n",index);
        list_add(head, &(base->page_link));
        core_array[index].nr_free ++;
        return;
    }
    while (1)
    {
        uintptr_t temp_pa = page2pa(le2page(temp,page_link));
        /* cprintf("temp_pa is %x, current_pa is %x\n",temp_pa, current_pa);
        if(temp_pa == 0x87ffc000 && current_pa == 0x87ff4000){
            cprintf("start assert\n");
            assert(temp_pa > current_pa);// the assert is right
        }*/
        if (temp_pa > current_pa){
            list_add_before(temp, &(base->page_link));
            break;
        }
        temp = temp->next;
        if(temp == head){
            list_add_before(head, &(base->page_link));
            break;
        }
    }
    core_array[index].nr_free ++;
    // if(index == 10){
    //     cprintf("call the add_page_into_list function with index 10, and the nr_free is %u\n",core_array[index].nr_free);
    // }
    merge_page(base,index);
    return;
}


static void
free_page_buddy(struct Page *base, size_t n){ // this method  handle the page which is smller or equal or bigger than 1024
// This is the main function of free page 
    assert(n > 0);
    int temp_index= -1;
    for(int i=MAX_NUM-1;i >=0;i--){
        if (tool_array[i] <= n){
            temp_index = i;
            break;
        }
    }
    if(temp_index == -1){
        temp_index = MAX_NUM - 1;
    }
    assert(temp_index >= 0);
    int rest = n - tool_array[temp_index];
    add_page_into_list(base,temp_index);
    cprintf("call the free_page_buddy function, and the rest is %u, and the temp_index is %u \n",rest,temp_index);
    if( rest > 0){
        free_page_buddy( base +  tool_array[temp_index], rest);
    }
    return;
}

static void
buddy_init_memmap(struct Page *base, size_t n)
{
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    cprintf("Start the buddy_init_memmap \n");
    free_page_buddy(base,n);

};

static void 
print_nr(){
    cprintf("the max 1024 has how many cores:%u\n",core_array[MAX_NUM - 1].nr_free);
    cprintf("the max 512 has how many cores:%u\n",core_array[MAX_NUM - 2].nr_free);
    cprintf("the max 256 has how many cores:%u\n",core_array[MAX_NUM - 3].nr_free);
    cprintf("the max 128 has how many cores:%u\n",core_array[MAX_NUM - 4].nr_free);
    cprintf("the max 64 has how many cores:%u\n",core_array[MAX_NUM - 5].nr_free);
    cprintf("the max 32 has how many cores:%u\n",core_array[MAX_NUM - 6].nr_free);
    cprintf("the max 16 has how many cores:%u\n",core_array[MAX_NUM - 7].nr_free);
    cprintf("the max 8 has how many cores:%u\n",core_array[MAX_NUM - 8].nr_free);
    cprintf("the max 4 has how many cores:%u\n",core_array[MAX_NUM - 9].nr_free);
    cprintf("the max 2 has how many cores:%u\n",core_array[MAX_NUM - 10].nr_free);
    cprintf("the max 1 has how many cores:%u\n",core_array[MAX_NUM - 11].nr_free);
};

static void
merge_page(struct Page *base, int index){ // handle the page that has been insert
    if(index >= MAX_NUM - 1){  
        return; 
    }
    uintptr_t current_pa = page2pa(base);
    uintptr_t pre_pa = NULL,next_pa = NULL;
    list_entry_t * head = &(core_array[index].free_list);
    if(base->page_link.prev != head){
        pre_pa = page2pa(le2page(base->page_link.prev,page_link));
    }
    if(base->page_link.next != head){
        next_pa = page2pa(le2page(base->page_link.next,page_link));
    }
    // if(current_pa == 0x87ff8000 && pre_pa == 0x87ff4000 && next_pa == 0x87ffc000){
    //     cprintf("pre_pa of  0x87ff8000 is %x \n",pre_pa);
    //     cprintf("next_pa of  0x87ff8000 is %x \n",next_pa);
    //     assert(head == (base->page_link.prev)->prev);
    //     cprintf("pre_pa of  0x87ff8000 is %x \n",page2pa(le2page((base->page_link.prev)->prev,page_link)));
    // }
    int shift_how_many_bit =  PAGE_BITS + 1;
    if(pre_pa != NULL && (pre_pa >> (shift_how_many_bit + index)) == (current_pa >> (shift_how_many_bit + index))){
        list_entry_t * _prev = base->page_link.prev;
        list_del(base->page_link.prev);
        core_array[index].nr_free--;
        list_del(&(base->page_link));
        core_array[index].nr_free--;
        ClearPageProperty(base);
        struct Page * pre_page = le2page(_prev,page_link);
        pre_page->property = pre_page->property * 2;
        add_page_into_list( le2page(_prev,page_link), index + 1);
        return;
    }
     if(next_pa != NULL && (next_pa >> (shift_how_many_bit + index)) == (current_pa >> (shift_how_many_bit + index))){
        list_entry_t * _next = base->page_link.next;
        list_del(base->page_link.next);
        core_array[index].nr_free--;
        list_del(&(base->page_link));
        core_array[index].nr_free--;
        ClearPageProperty(le2page(_next,page_link));
        base->property = base->property * 2;
        add_page_into_list( base, index + 1);
        return;
    }
    return;
}
static struct Page *
_buddy_alloc_pages_by_index(int index){
    assert(core_array[index].nr_free > 0);
    list_entry_t * to_return = core_array[index].free_list.next;
    list_del(to_return);
    core_array[index].nr_free --;
    return le2page(to_return, page_link);
}

static struct Page *
buddy_alloc_pages(size_t n)
{
    cprintf("call the buddy_alloc_pages function and the size is %u\n",n);
    assert(n > 0);
    if( n > tool_array[MAX_NUM - 1]){
        cprintf("To big page alloc one time\n");
        return NULL;
    }
    struct Page *page = NULL;
    int temp_index= -1;
    for(int i=0;i < MAX_NUM ;i++){
        if (tool_array[i] >= n && core_array[i].nr_free > 0){  //find the bigger or equal the n
            temp_index = i;
            break;
        }
    }
    if (temp_index == -1){
        cprintf("No enough physical memory to alloc\n");
        return NULL;
    }
    cprintf("in buddy_alloc_pages function, the temp_index is %u\n",temp_index);
    page = _buddy_alloc_pages_by_index(temp_index);
    if(tool_array[temp_index] == n){
        ClearPageProperty(page);
        cprintf("sucessfully return the buddy_alloc_pages function with the page size is fine and the size is %u\n",n);
        return page; // Which means that it just proper;
    }else 
    {
        assert(n < tool_array[temp_index]);
        struct Page * page_to_free = page + n;
        page_to_free->property = page->property - n;
        page->property = n;
        cprintf("in buddy_alloc_pages function, the page_to_free property is %u\n",page_to_free->property);
        free_page_buddy(page_to_free,  page_to_free->property);
        ClearPageProperty(page);
    }
    cprintf("sucessfully return the buddy_alloc_pages function and the size is %u\n",n);
    return page;
}


static void
buddy_free_pages(struct Page *base, size_t n)
{
    cprintf("call the buddy_free_pages function with free size is %u\n",n);
    assert(n > 0);
    assert( n < tool_array[MAX_NUM-1]);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
   free_page_buddy(base,n);
    
}

static size_t
buddy_nr_free_pages(void)
{
    size_t nr_free = 0;
    for( int i=0; i< MAX_NUM; i++){
        nr_free = nr_free + tool_array[i] * core_array[i].nr_free;
    }
    return nr_free;
}

static void
basic_check(void)
{   
    print_nr();
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);
    print_nr();
    cprintf("p0 physical address %x \n",page2pa(p0));
    cprintf("p1 physical address %x \n",page2pa(p1));
    cprintf("p2 physical address %x \n",page2pa(p2));
    
    free_page(p0);
    print_nr();
    free_page(p1);
    print_nr();
    free_page(p2);
    print_nr();
}

// below code is used to check the best fit allocation algorithm (your EXERCISE 1)
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
buddy_check(void)
{
    cprintf("Start the buddy check: -------------------------------- \n");
    basic_check();
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages =buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
