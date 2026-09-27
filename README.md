# redis-lite


## Hashtable
Collision Resolution: If two keys produces same hash index in hashtable the techniue used to resolve these issues 
    are called as collision resolution.

Types of Hashtable (Depending on type of collision technique):
a) Open addressing.
b) Chaining.

a) Open addressing: 
    Open addressing seeks another slot in case of the collision.
Advantages:
    - No extra linked-list
    - Better cache locality.
    - Often faster in practice when the laod factor is kept resonably low.
    - Memory layout is simple.
    - No pointer is required for chaining.

b) Chaning:
    Chaning group conflicting keys using linked list.
Advantages:
    - Simple collision handling - just add the new element to the chain.
    - Can store more elements than the table size - the talbe doesn't have to be resized.
    - Deletion is easier - remove the element from its chain without worrying about the reorganizing the elements.
    - Performance degrades relatively gracefully as the laod factor increases.
    - Less sensitive to clustering than open addressing.


Interview: I chose separate chaining because Redis-lite needs frequent GET, SET and DELETE operations. Chaining makes deletion straightforward and allows the hash table to tolerate higher load factors without requiring every slot to contain an element. The tradeoff is additional memory and potentially poorer cache locality compared with open addressing.


Q) h_lookup return HNode** & hm_loopup return HNode*?
=> h_lookup is lower level function it reutrn the address of the pointer pointing to node so that if we want to delete the node we can do it by modifying the pointer.
but in case of hm_lookup it more high-level function where user just want the address of the node so that it can use that node.

Q) hmap -> old = HMAP{}; is these like malloc or what?
=> It just assgin the defaul the values to the members of the object.
