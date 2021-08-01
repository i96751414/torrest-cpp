try:
    from urllib import request as ul
except ImportError:
    # noinspection PyUnresolvedReferences
    import urllib2 as ul


def get_mimes():
    html = ul.urlopen("https://svn.apache.org/repos/asf/httpd/httpd/trunk/docs/conf/mime.types").read().decode()
    for line in html.splitlines():
        if not line.startswith("#"):
            mime, *extensions = line.split()
            for ext in extensions:
                print('{".%s", "%s"},' % (ext, mime))


if __name__ == "__main__":
    get_mimes()
