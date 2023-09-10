# Makefile示例

# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -Werror -lpthread -g

# 源文件和目标文件
SRCS = threadpool.c client.c #resource.c
OBJS = $(SRCS:.c=.o)

# 目标文件
TARGET = client

# 默认目标，即直接运行make命令时执行的目标
all: $(TARGET)

# 生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 编译每个源文件为目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# 清理生成的目标文件和可执行文件
clean:
	rm -f $(OBJS) $(TARGET)

