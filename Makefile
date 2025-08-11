NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRCDIR = src
INCDIR = includes
OBJDIR = obj

SOURCES = CGI.cpp \
		  Config.cpp \
		  HttpRequest.cpp \
		  HttpResponse.cpp \
		  LocationConfig.cpp \
		  main.cpp \
		  ServerConfig.cpp \
		  utils.cpp \
		  WebServer.cpp

OBJECTS = $(SOURCES:%.cpp=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
