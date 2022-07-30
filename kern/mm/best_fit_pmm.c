#include <pmm.h>
#include <list.h>
#include <string.h>
#include <best_fit_pmm.h>
#include <stdio.h>

free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

static void
best_fit_init(void)
{
    list_init(&free_list);
    nr_free = 0;
}

static void
best_fit_init_memmap(struct Page *base, size_t n)
{
    cprintf("in best_fit_init_memmap, n is %u \n ", n);
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
        list_entry_t* temp = free_list.prev;
        while (temp != &free_list)
        {
            cprintf("page2pa(page):%x\n",page2pa(le2page(temp,page_link)));
            temp = temp->prev;
        }
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
            cprintf("page2pa(page):%x\n",page2pa(page));
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }

}

static struct Page *
best_fit_alloc_pages(size_t n)
{
    assert(n > 0);
    if (n > nr_free) {
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;

    int min_distance = 2147483647;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n && p->property - n < min_distance) {
            min_distance = p->property - n;
            page = p;
        }
    }
    if (page != NULL) {
        list_entry_t* prev = list_prev(&(page->page_link));
        list_del(&(page->page_link));
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            SetPageProperty(p);  // set means the header
            list_add(prev, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;

}

static void
best_fit_free_pages(struct Page *base, size_t n)
{
 assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }

    list_entry_t* le = list_prev(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            list_del(&(base->page_link));
            base = p;
        }
    }

    le = list_next(&(base->page_link));
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }
    
}

static size_t
best_fit_nr_free_pages(void)
{
    return nr_free;
}

static void
basic_check(void)
{
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));
    
    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

// below code is used to check the best fit allocation algorithm (your EXERCISE 1)
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
best_fit_check(void)
{
    int score = 0, sumscore = 6;
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list)
    {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();
    
    struct Page *p0 = alloc_pages(12), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    cprintf("nr_free_store : %u\n",nr_free_store);
    nr_free = 0;

    free_pages(p0 + 2, 3);
    free_pages(p0 + 5, 2);
    cprintf("p0+5 ppa: %x\n", page2pa(p0 + 5));
    cprintf("p0+2 ppa: %x\n", page2pa(p0 + 2));
    p1 = alloc_pages(2);
    assert(p1 == p0 + 5);
    p2 = alloc_pages(3);
    assert(p2 == p0 + 2);
    assert((&free_list)->next != &free_list);

    nr_free = nr_free_store;
    free_list = free_list_store;

    le = &free_list;
    assert((&free_list)->next->next == &free_list);
    free_pages(p0, 12);
    assert((&free_list)->next != &free_list);
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);

// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
//     struct Page *p0 = alloc_pages(5), *p1, *p2;
//     assert(p0 != NULL);
//     assert(!PageProperty(p0));

// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
//     list_entry_t free_list_store = free_list;
//     list_init(&free_list);
//     assert(list_empty(&free_list));
//     assert(alloc_page() == NULL);

// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
//     unsigned int nr_free_store = nr_free;
//     nr_free = 0;

//     // * - - * -
//     free_pages(p0 + 1, 2);
//     free_pages(p0 + 4, 1);
//     assert(alloc_pages(4) == NULL);
//     assert(PageProperty(p0 + 1) && p0[1].property == 2);
//     // * - - * *
//     assert((p1 = alloc_pages(1)) != NULL);
//     assert(alloc_pages(2) != NULL); 
//     assert(p0 + 4 == p1);

// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
//     p2 = p0 + 1;
//     free_pages(p0, 5);
//     assert((p0 = alloc_pages(5)) != NULL);
//     assert(alloc_page() == NULL);

// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
//     assert(nr_free == 0);
//     nr_free = nr_free_store;

//     free_list = free_list_store;
//     free_pages(p0, 5);

//     le = &free_list;
//     while ((le = list_next(le)) != &free_list)
//     {
//         struct Page *p = le2page(le, page_link);
//         count--, total -= p->property;
//     }
//     assert(count == 0);
//     assert(total == 0);
// #ifdef ucore_test
//     score += 1;
//     cprintf("grading: %d / %d points\n", score, sumscore);
// #endif
}

const struct pmm_manager best_fit_pmm_manager = {
    .name = "best_fit_pmm_manager",
    .init = best_fit_init,
    .init_memmap = best_fit_init_memmap,
    .alloc_pages = best_fit_alloc_pages,
    .free_pages =best_fit_free_pages,
    .nr_free_pages = best_fit_nr_free_pages,
    .check = best_fit_check,
};
