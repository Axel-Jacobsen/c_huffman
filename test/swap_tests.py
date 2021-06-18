#! /usr/bin/env python3

def swap(arr, lidx, slidx, max_idx):
    if lidx  != max_idx - 1:
        tmp = arr[lidx]
        arr[lidx] = arr[max_idx - 1]
        arr[max_idx - 1] = tmp
    if slidx != max_idx - 2:
        tmp = arr[slidx]
        arr[slidx] = arr[max_idx - 2]
        arr[max_idx - 2] = tmp

def swap_better(arr, lidx, slidx, max_idx):
    # swap 1
    tmp = arr[lidx]
    arr[lidx] = arr[max_idx - 1]
    arr[max_idx - 1] = tmp

    # swap 2
    if slidx == max_idx - 1:
        slidx = lidx
    tmp = arr[slidx]
    arr[slidx] = arr[max_idx - 2]
    arr[max_idx - 2] = tmp


if __name__ == "__main__":
    arr = [c for c in "XYcdefgh"]
    print(f"goal: want X,Y in last 2 positions of {arr}, like [..., 'Y', 'X']")
    swap(arr, 0, 1, len(arr))
    print(f"swap(arr, 0, 1, len(arr)) -> {arr}")
    arr = [c for c in "YbcdefgX"]
    print(f"works! But what if the array is {arr}...")
    swap(arr, 0, len(arr)-1, len(arr))
    print(f"we get {arr}")
    print(f"swap(arr, 0, len(arr)-1, len(arr)) doesn't work But what if slidx == max_idx - 1?")
    arr = [c for c in "YbcdefgX"]
    print(f"using swap_better(arr, 0, len(arr)-1, len(arr)) on {arr}...")
    swap_better(arr, len(arr)-1, 0, len(arr))
    print(f"we get {arr}")
