#include "kfifo.h"

struct kfifo *dws_warn_fifo;

struct kfifo *kfifo_init(unsigned char *buffer, unsigned int size)
{
    struct kfifo *fifo;

    BUG_ON(!is_power_of_2(size));

    fifo = malloc(sizeof(struct kfifo));
    if (!fifo)
        return (void*)(-ENOMEM);

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;

    return fifo;
}


struct kfifo *kfifo_alloc(unsigned int size)
{
    unsigned char *buffer;
    struct kfifo *ret;

    if (!is_power_of_2(size)) {
        BUG_ON(size > 0x80000000);
        size = roundup_pow_of_two(size);
    }

    buffer = malloc(size);

    if (!buffer)
        return (void *)(-ENOMEM);

    ret = kfifo_init(buffer, size);

    if ((unsigned long)ret <= 0)
    {
        free(buffer);
    }

    return ret;
}

void kfifo_free(struct kfifo *fifo)
{
    free(fifo->buffer);
    free(fifo);
}

unsigned int __kfifo_put(struct kfifo *fifo, unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->size - fifo->in + fifo->out);
    l = min(len, fifo->size - (fifo->in & (fifo->size - 1)));

    memcpy(fifo->buffer + (fifo->in & (fifo->size - 1)), buffer, l);
    memcpy(fifo->buffer, buffer + l, len - l);

    fifo->in += len;

    return len;
}

unsigned int __kfifo_get(struct kfifo *fifo,
             unsigned char *buffer, unsigned int len)
{
    unsigned int l;

    len = min(len, fifo->in - fifo->out);
    l = min(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy(buffer, fifo->buffer + (fifo->out & (fifo->size - 1)), l);
    memcpy(buffer+l, fifo->buffer, len-l);

    fifo->out += len;
    return len;
}
