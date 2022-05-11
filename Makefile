NAME                := webserv

SRCS                := \
                       class.cpp \
                       main.cpp

OBJS                := $(SRCS:.cpp=.o)

CXX                 := c++
CXXFLAGS            := -std=c++98 -Wall -Wextra -Werror

INCLUDE             := -I .

LIBRARY             :=

COMPILE             = $(CXX) $(CXXFLAGS) $(INCLUDE) $(DEBUG_OPTION)
LINK                = $(CXX) $(CXXFLAGS) $(INCLUDE) $(DEBUG_OPTION) $(LIBRARY)

RM                  := rm -f



.PHONY:             all debug setdebug clean fclean re
all:                $(NAME)

debug:              clean setdebug $(NAME)
setdebug:
	$(eval DEBUG_OPTION = -g)

clean:
	$(RM) $(OBJS)
fclean:             clean
	$(RM) $(NAME)
re:                 fclean all



$(NAME):            $(OBJS)
	$(LINK) -o $@ $^

%.o:                %.cpp
	$(COMPILE) -o $@ -c $<
