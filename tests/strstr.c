//Yeah, strstr actually returns a char* and this thing is structured
//contrivedly. It's just to test break and casting.

int badstrstr (char* haystack, char* needle) {
    char* original = haystack;
    
    for (; haystack[0] != (char) 0; haystack++) {
        for (int i = 0; ; i++) {
            if (needle[i] == (char) 0)
                return (int)(haystack-original);
            
            else if (haystack[i] != needle[i])
                break;
        }
    }
    
    return -1;
}

int main () {
    char* haystack = "hell hello";
    char* needle = haystack + (char*) 5;
    
    return badstrstr(haystack, needle);
}