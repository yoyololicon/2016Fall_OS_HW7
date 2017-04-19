#include <iostream>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
using namespace std;

enum dir {current, parent, sub};

class op
{
public:
    op()
    {
        inode = size_max = size_min = 0;
        match = 0;
        dir = current;
        file_name.clear();
        path.clear();
    }
    ~op(){};
    void get_parent_dir(char *p)
    {
        path = string(p);
        dir = parent;
    }
    void get_child_dir(char *p)
    {
        path = string(p);
        dir = sub;
    }
    void get_inode(char *arg)
    {
        inode = atoi(arg);
    }
    void get_name(char *arg)
    {
        file_name = string(arg);
    }
    void get_min(char *arg)
    {
        size_min = atoi(arg);
    }
    void get_max(char *arg)
    {
        size_max = atoi(arg);
    }
    void caculate_match()
    {
        match = 0;
        if(inode)
            match++;
        if(file_name.size())
            match++;
        if(size_min)
            match++;
        if(size_max)
            match++;
    }
    int dir, inode, size_min, size_max, match;
    string file_name, path;
};

void find_recursive(const char *name, op* options)
{
    DIR *stream;
    struct dirent *ddir;

    stream = opendir(name);
    if(stream == NULL)
        return;

    int total_match = options->match;
    char path[1024];
    int len;
    struct stat sb;
    double size;

    while(ddir = readdir(stream))
    {
        switch (ddir->d_type)
        {
            case DT_REG:
                if(options->inode)
                {
                    if(options->inode == ddir->d_ino)
                        total_match--;
                }
                if(options->file_name.size())
                {
                    if(!strcmp(options->file_name.c_str(), ddir->d_name))
                        total_match--;
                }
                if(options->size_max)
                {
                    len = snprintf(path, 1023, "%s/%s", name, ddir->d_name);
                    path[len] = 0;
                    if(stat(path, &sb) == -1)
                    {
                        perror("stat");
                        return;
                    }
                    size = sb.st_size;
                    size /= 1048576;

                    if(size <= options->size_max)
                        total_match--;
                }
                if(options->size_min)
                {
                    len = snprintf(path, 1023, "%s/%s", name, ddir->d_name);
                    path[len] = 0;
                    if(stat(path, &sb) == -1)
                    {
                        perror("stat");
                        return;
                    }
                    size = sb.st_size;
                    size /= 1048576;

                    if(size >= options->size_min)
                        total_match--;
                }
                if(!total_match)
                {
                    len = snprintf(path, 1023, "%s/%s", name, ddir->d_name);
                    path[len] = 0;
                    if(stat(path, &sb) == -1)
                    {
                        perror("stat");
                        return;
                    }
                    size = sb.st_size;
                    size /= 1048576;

                    len = snprintf(path, 1023, "%s/%s %ld %f MB", name, ddir->d_name, ddir->d_ino, size);
                    path[len] = 0;
                    cout << string(path) << endl;
                }
                total_match = options->match;
                break;
            case DT_DIR:
                if(strcmp(ddir->d_name, ".") == 0 || strcmp(ddir->d_name, "..") == 0)
                    continue;
                len = snprintf(path, 1023, "%s/%s", name, ddir->d_name);
                path[len] = 0;
                find_recursive(path, options);
                break;
            default:
                cout << "not a file" << endl;
                break;
        }
    }
    closedir(stream);
}

int main(int argc, char **argv)
{
    op options;
    string path;

    if(argc < 2)
    {
        cout << "my_find [pathname][options]" << endl;
        return 1;
    }
    else
    {
        if(strncmp(argv[1], "../", 3) == 0)
        {
            options.get_parent_dir(argv[1]);
            argc--;
            argv++;
        }
        else if(strncmp(argv[1], "./", 2) == 0)
        {
            options.get_child_dir(argv[1]);
            argc--;
            argv++;
        }
        int c;

        while (1)
        {
            int option_index = 0;
            static struct option long_options[] = {
                    {"inode", required_argument, NULL, 0},
                    {"name", required_argument, NULL, 1},
                    {"size_min", required_argument, NULL, 2},
                    {"size_max", required_argument, NULL, 3},
                    {0, 0, 0, 0}
            };

            c = getopt_long_only(argc, argv, "", long_options, &option_index);
            if(c == -1)
                break;

            switch (c)
            {
                case 0:
                    options.get_inode(optarg);
                    break;
                case 1:
                    options.get_name(optarg);
                    break;
                case 2:
                    options.get_min(optarg);
                    break;
                case 3:
                    options.get_max(optarg);
                    break;
                default:
                    return 1;
            }
        }
    }

    options.caculate_match();

    if(options.dir > current)
        find_recursive(options.path.c_str(), &options);
    else
        find_recursive(".", &options);

    return 0;
}