#ifndef LIBPAS_DSTRUCTURE_AUTO_H
#define LIBPAS_DSTRUCTURE_AUTO_H

#define FREE_LIST(list) free((list)->item); (list)->item=NULL; (list)->length=0;

#define FUN_LIST_INIT(T) int init ## T ## List(T ## List *list, unsigned int n){list->item = (T *) malloc(n * sizeof *(list->item));if (list->item == NULL) {return 0;}return 1;}
#define FUN_LIST_GET_BY(V,T) T *get##T##By_##V (int id, const T##List *list) {  LIST_GET_BY(V) }
#define FUN_LIST_GET_BY_ID(T) T *get ## T ## ById(int id, const T ## List *list) {  LIST_GET_BY_ID }
#define FUN_LIST_GET_BY_IDSTR(T) T *get ## T ## ById(char *id, const T ## List *list) {  LIST_GET_BY_IDSTR }
#define FUN_LLIST_GET_BY_ID(T) T *get ## T ## ById(int id, const T ## List *list) {  LLIST_GET_BY_ID(T) }

#define DEF_FUN_LIST_INIT(T)  int init ## T ## List(T ## List *list, unsigned int n);
#define DEF_FUN_LIST_GET_BY_ID(T) extern T *get ## T ## ById(int id, const T ## List *list);
#define DEF_FUN_LIST_GET_BY_IDSTR(T) extern T *get ## T ## ById(char *id, const T ## List *list);
#define DEF_FUN_LLIST_GET_BY_ID(T) extern T *get ## T ## ById(int id, const T ## List *list);

#define DEF_LIST(T) typedef struct {T *item; size_t length;} T##List;
#define DEF_LLIST(T) typedef struct {T *top; T *last; size_t length;} T##List;


#define DEF_FIFO_LIST(T) struct fifo_item_ ## T {T data;int free;struct fifo_item_ ## T *prev;struct fifo_item_ ## T *next;};typedef struct fifo_item_ ## T FIFOItem_ ## T;typedef struct {FIFOItem_ ## T *item;size_t length;FIFOItem_ ## T *push_item;FIFOItem_ ## T *pop_item;Mutex mutex;} FIFOItemList_ ## T;
#define FUN_FIFO_PUSH(T) int T ## _fifo_push(T item, FIFOItemList_ ## T *list) {if (!lockMutex(&list->mutex)) {return 0;}if (list->push_item == NULL) {unlockMutex(&list->mutex);return 0;}list->push_item->data = item;list->push_item->free = 0;if(list->pop_item==NULL){list->pop_item=list->push_item;}if (list->push_item->next->free) {list->push_item = list->push_item->next;} else {list->push_item = NULL;}unlockMutex(&list->mutex);return 1;}
#define FUN_FIFO_POP(T) int T ## _fifo_pop(T * item, FIFOItemList_ ## T *list) {if (!lockMutex(&list->mutex)) {return 0;}if (list->pop_item == NULL) {unlockMutex(&list->mutex);return 0;}*item = list->pop_item->data;list->pop_item->free = 1;if (list->push_item == NULL) {list->push_item = list->pop_item;}if (!list->pop_item->next->free) {list->pop_item = list->pop_item->next;} else {list->pop_item = NULL;}unlockMutex(&list->mutex);return 1;}
#define FREE_FIFO(fifo) FREE_LIST(fifo);(fifo)->pop_item = NULL;(fifo)->push_item = NULL;
#define DEF_FUN_FIFO_PUSH(T) extern int T ## _fifo_push(T item, FIFOItemList_ ## T *list);
#define DEF_FUN_FIFO_POP(T) extern int T ## _fifo_pop(T * item, FIFOItemList_ ## T *list);

#endif 

