# Motivation

I was reading CSAPP book. Specifically about malloc internals. So i decided to implement my own. 

# Problems to solve

So...dynamic memory allocator. Sounds cool. But it has to solve a lot of complex problems to work not only properly but fast as possible with maximum memory utilization (where niggalloc sucks)

### 1 Free block organization:
How to store information about which blocks are allocated and which are free? We have to somehow organize them. But how? Well we can do it inside allocated memory. 
 
 **BLOCK FORMAT**
 ```  
    31             3     0
    +-------------------+
    + BLOCK SIZE   |A|F + (HEADER)
    +-------------------+
    + PAYLOAD           +
    +-------------------+
    + PADDING           +
    +-------------------+
    + FOOTER            +
    +-------------------+
```
 
 **HEADER**
 ```
 29                            3
11111111 1111111 1111111 11111 111
|____________________________| |_|
          BLOCK SIZE         INFO BITS
```
   
 Block format is general idea how to divide heap into blocks. 

 **Header** -> general information about block
 **Block size** -> how many bytes occupies block (including header and footer)
 **Info bits**    -> is block allocated (1) or free (0)
  **Payload** -> actual requested data
  **Padding** -> aligning (8 bytes)
  **Footer** -> Identical copy of header 

### 2 Placement:
  How to find proper free blocks?
  
  Well, there are a lot of techniques, including **buddy system**, **best fit** and so on. I decided to use the simplest approach. **first fit**. It simply looks from base heap pointer through all blocks until it finds free block that is large enough to hold allocated request

### 3 Coalescing:
 What should we do after we free block?
 Well..U see sometimes we have also free neighbors. Why don't we join them together to create one big free block? 

## TODO
1) memory utilization upgrade
2) add zero_niggalloc (calloc)
## Inspiration
Terry A Davis
Donald Trump 