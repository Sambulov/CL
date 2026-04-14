#include "CodeLib.h"

typedef struct __LL_ITEM_T {
	uint32_t ovn    :31,
	         marker :1;
	struct __LL_ITEM_T *next;
	struct __LL_ITEM_T *prev;
	struct __LL_ITEM_T **root;
} __LinkedListItem_t;

LIB_ASSERRT_STRUCTURE_CAST(__LinkedListItem_t, LinkedListItem_t, LL_ITEM_SIZE, LinkedList.h);

#define _bIsValidItem(it)         (((it) != libNULL) && ((it)->ovn == (((uint32_t)(it)) & 0x7fffffffUL)))

void vLinkedListUnlink(LinkedListItem_t *pxItem) {
	__LinkedListItem_t *item = (__LinkedListItem_t *)pxItem;
	if (_bIsValidItem(item)) {
		if (item->root != libNULL) {
			if (*(item->root) == item) {
				__LinkedListItem_t * nextListItem = item->next;
				while (nextListItem->root != item->root)
				    nextListItem = nextListItem->next;
				if (nextListItem == item)
					*item->root = libNULL;
				else
					*item->root = nextListItem;
			}
			__LinkedListItem_t* prevItem = item->prev;
			__LinkedListItem_t* nextItem = item->next;
			prevItem->next = item->next;
			nextItem->prev = item->prev;
		}
	}
	if(item != libNULL) {
		item->ovn = (uint32_t)item;
		item->marker = 0;
		item->next = item;
		item->prev = item;
		item->root = libNULL;
	}
}

void vLinkedListInsert(LinkedList_t *pxList, LinkedListItem_t *pxItem, ListItemComparer_t pfCmp) {
	if ((pxItem == libNULL) || (pxList == libNULL))
		return;
	__LinkedListItem_t *newItem = (__LinkedListItem_t *)pxItem;
	vLinkedListUnlink(pxItem);
	if (*pxList == libNULL) {
		newItem->root = (__LinkedListItem_t **)pxList;
		*pxList = pxItem;
	}
	else {
		__LinkedListItem_t *listCurrentItem = (__LinkedListItem_t *)*pxList;
		if (!_bIsValidItem(listCurrentItem)) 
			return;
		if(!listCurrentItem->root)
			listCurrentItem->root = (__LinkedListItem_t **)pxList;
		newItem->root = listCurrentItem->root;
		if (pfCmp != libNULL) {
			__LinkedListItem_t **pxRoot = listCurrentItem->root;
			listCurrentItem = *pxRoot;
			while (pfCmp(pxItem, (LinkedListItem_t *)listCurrentItem) > 0) {
				listCurrentItem = listCurrentItem->next;
				if (listCurrentItem == *pxRoot)
					break;
			}
		}
		/* insert before current */
		__LinkedListItem_t* itemBeforeCurrent = listCurrentItem->prev;
		newItem->next = listCurrentItem;
		newItem->prev = itemBeforeCurrent;
		listCurrentItem->prev = newItem;
		itemBeforeCurrent->next = newItem;
	}
}

uint8_t bLinkedListContains(LinkedList_t xList, LinkedListItem_t *pxItem) {
	__LinkedListItem_t *pxItemPr = (__LinkedListItem_t *)pxItem;
	__LinkedListItem_t *pxListPr = (__LinkedListItem_t *)xList;
	return _bIsValidItem(pxListPr) && _bIsValidItem(pxItemPr) && 
		pxListPr->root && (pxListPr->root == pxItemPr->root);	
}

static LinkedListItem_t *_pxLinkedListFind(__LinkedListItem_t *pxCurrentItem, LinkedListMatch_t pfMatch, void *pxMatchArg, uint8_t bFirst, uint8_t bOverlap) {
  if (_bIsValidItem(pxCurrentItem)) {
    __LinkedListItem_t *first = *(__LinkedListItem_t **)pxCurrentItem->root;
    if((first == libNULL) || !bFirst) {
      first = pxCurrentItem->next;
      if(((first == libNULL) || (first == *(__LinkedListItem_t **)pxCurrentItem->root)) && !bOverlap)
        return libNULL;
      }
    __LinkedListItem_t *last = first->prev;
    if(!bOverlap && (pxCurrentItem->root != libNULL))
      last = (*(__LinkedListItem_t **)pxCurrentItem->root)->prev;
	pxCurrentItem = first;
	while (1) {
		if ((pfMatch == libNULL) || pfMatch((LinkedListItem_t *)pxCurrentItem, pxMatchArg))
			return (LinkedListItem_t *)pxCurrentItem;
		if (pxCurrentItem == last)
			break;
		pxCurrentItem = pxCurrentItem->next;
	}
  }
  return libNULL;
}

LinkedListItem_t *pxLinkedListFindFirst(LinkedList_t xList, LinkedListMatch_t pfMatch, void *pxMatchArg) {
	return _pxLinkedListFind((__LinkedListItem_t *)xList, pfMatch, pxMatchArg, 1, 0);
}

LinkedListItem_t *pxLinkedListFindNextOverlap(LinkedListItem_t *pxCurrentItem, LinkedListMatch_t pfMatch, void *pxMatchArg) {
	return _pxLinkedListFind((__LinkedListItem_t *)pxCurrentItem, pfMatch, pxMatchArg, 0, 1);
}

LinkedListItem_t *pxLinkedListFindNextNoOverlap(LinkedListItem_t *pxCurrentItem, LinkedListMatch_t pfMatch, void *pxMatchArg) {
	return _pxLinkedListFind((__LinkedListItem_t *)pxCurrentItem, pfMatch, pxMatchArg, 0, 0);
}

uint32_t ulLinkedListDoForeach(LinkedList_t xList, LinkedListAction_t fAction, void *pxArg) {
	__LinkedListItem_t *item = (__LinkedListItem_t *)xList;
	uint32_t amount = 0;
	if (fAction && _bIsValidItem(item)) {
		void **pxRoot = (void **)item->root;
		if(!*pxRoot) 
			*pxRoot = item; /* it is unlinked, single item */
		__LinkedListItem_t *currentItem = *pxRoot;
		do {
			void *nextItem = currentItem->next;
			if(nextItem == *pxRoot)
				nextItem = libNULL;
			if(!currentItem->marker) {
				currentItem->marker = 1;
				fAction((LinkedListItem_t *)currentItem, pxArg);
				amount++;
			}
			currentItem = nextItem;
			if(currentItem && ((void **)currentItem->root != pxRoot))
				currentItem = *pxRoot;
		} while (currentItem);
		currentItem = *pxRoot;
		if(currentItem) do {
			currentItem->marker = 0;
			currentItem = currentItem->next;
		} while (currentItem != *pxRoot);
	}
	return amount;
}

typedef struct {
	LinkedListMatch_t fMatch;
	void *pvMatchArg;
	uint32_t ulCounter;
} __CounterArgTuple_t;

static inline void _vCountForeachWrap(LinkedListItem_t *pxItem, void *pxArg) {
	__CounterArgTuple_t *arg = (__CounterArgTuple_t *)pxArg;
	if ((arg->fMatch == libNULL) || (arg->fMatch(pxItem, arg->pvMatchArg))) {
		(arg->ulCounter)++;
	}
}

uint32_t ulLinkedListCount(LinkedList_t xList, LinkedListMatch_t pfMatch, void *pxMatchArg) {
	__CounterArgTuple_t arg = { .fMatch = pfMatch, .pvMatchArg = pxMatchArg, .ulCounter = 0 };
	ulLinkedListDoForeach(xList, &_vCountForeachWrap, &arg);
	return arg.ulCounter;
}

static inline void _vUnlinkForeachWrap(LinkedListItem_t *pxItem, void *pxArg) {
	(void)pxArg;
	__LinkedListItem_t *item = (__LinkedListItem_t *)pxItem;
	if(item->root == pxArg)
		vLinkedListUnlink(pxItem);
}

void vLinkedListClear(LinkedList_t *pxList) {
	if(pxList) {
		ulLinkedListDoForeach(*pxList, &_vUnlinkForeachWrap, pxList);
		*pxList = libNULL;
	}
}

void vLinkedListClearItemList(LinkedListItem_t *pxItem) {
	if(pxItem)
		vLinkedListClear((LinkedList_t *)((__LinkedListItem_t *)pxItem)->root);
}


linked_list_item_t *linked_list_find_first(linked_list_t, linked_list_match_t, void *)\
                                                      __attribute__ ((alias ("pxLinkedListFindFirst")));

linked_list_item_t *linked_list_find_next_overlap(linked_list_item_t *, linked_list_match_t, void *)\
                                                      __attribute__ ((alias ("pxLinkedListFindNextOverlap")));

linked_list_item_t *linked_list_find_next_no_overlap(linked_list_item_t *, linked_list_match_t, void *)\
                                                      __attribute__ ((alias ("pxLinkedListFindNextNoOverlap")));

uint32_t linked_list_do_foreach(linked_list_t, linked_list_action_t, void *)\
                                                      __attribute__ ((alias ("ulLinkedListDoForeach")));

void linked_list_insert(linked_list_t *, linked_list_item_t *, list_item_comparer_t)\
                                                      __attribute__ ((alias ("vLinkedListInsert")));

void linked_list_insert_last(linked_list_t *, linked_list_item_t *)\
                                                      __attribute__ ((alias ("vLinkedListInsertLast")));

void linked_list_unlink(linked_list_item_t *)     __attribute__ ((alias ("vLinkedListUnlink")));

uint32_t linked_list_count(linked_list_t, linked_list_match_t, void *)\
                                                      __attribute__ ((alias ("ulLinkedListCount")));

void linked_list_clear(linked_list_t *) __attribute__ ((alias ("vLinkedListClear")));
void linked_list_clear_item_list(linked_list_item_t *item) __attribute__ ((alias ("vLinkedListClearItemList")));

uint8_t linked_list_contains(linked_list_t, linked_list_item_t *)\
                                                      __attribute__ ((alias ("bLinkedListContains")));
