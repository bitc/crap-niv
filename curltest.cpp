#include <curl/curl.h>

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    printf("%d %d\n", size, nmemb);
    return size * nmemb;
}

int main(int argc, char *argv[])
{
    if(curl_global_init(CURL_GLOBAL_ALL))
        throw 0;

    CURL *easyhandle = curl_easy_init();
    if (!easyhandle)
        throw 0;

    curl_easy_setopt(easyhandle, CURLOPT_URL, argv[1]);
    curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, NULL);

    CURLM *multihandle = curl_multi_init();
    if (!multihandle)
        throw 0;


    curl_multi_add_handle(multihandle, easyhandle);

    //curl_easy_perform(easyhandle);
    int running_handles;
    do
    {
        curl_multi_perform(multihandle, &running_handles);
    } while (running_handles > 0);

    curl_multi_remove_handle(multihandle, easyhandle);
    curl_easy_cleanup(easyhandle);
    curl_multi_cleanup(multihandle);
}

