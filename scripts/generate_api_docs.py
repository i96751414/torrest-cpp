import os
from collections import namedtuple, OrderedDict
from operator import attrgetter

import requests

Endpoint = namedtuple("Endpoint", ("path", "method", "summary", "description", "parameters", "responses"))
Parameter = namedtuple("Parameter", ("name", "location", "description", "required", "schema"))
Response = namedtuple("Response", ("code", "description"))


def filter_items(data, *keys):
    sentinel = object()
    for k in keys:
        v = data.get(k, sentinel)
        if v is not sentinel:
            yield k, v


def parse_oas(spec):
    return [Endpoint(
        path, method.upper(),
        details.get("summary", "") or path_item.get("summary", ""),
        details.get("description", "") or path_item.get("description", ""),
        [Parameter(
            param["name"], param["in"], param.get("description", ""),
            param.get("required", False), param.get("schema", {}).get("type", "object")
        ) for param in details.get("parameters", [])],
        [Response(code, response.get("description")) for code, response in details.get("responses", {}).items()],
    ) for path, path_item in spec.get("paths", {}).items() for method, details in filter_items(
        path_item, "get", "put", "post", "delete", "options", "head", "patch", "trace")]


def get_section(path):
    if "{file}" in path:
        return "Files"
    elif path.startswith("/torrents"):
        return "Torrents"
    elif path.startswith("/settings"):
        return "Settings"
    else:
        return "Service"


def make_markdown_table(array):
    widths = [max(len(line[i]) for line in array) for i in range(len(array[0]))]
    widths = [max(w, 3) for w in widths]
    array = [[elt.ljust(w) for elt, w in zip(line, widths)] for line in array]
    array_head, *array_body = array
    header = "| " + " | ".join(array_head) + " |"
    separator = "|-" + "-|-".join(["-" * w for w in widths]) + "-|"
    body = [""] * len(array)
    for idx, line in enumerate(array[1:]):
        body[idx] = "| " + " | ".join(line) + " |"

    return header + "\n" + separator + "\n" + "\n".join(body) + "\n"


def create_md(fd, endpoints_by_section, version, description=""):
    fd.write("# Rest API\n\n## Version: {}\n".format(version))
    if description:
        fd.write("\n" + description + "\n")
    for section, endpoints in endpoints_by_section.items():
        fd.write("\n#### {}\n\n".format(section))

        for endpoint in endpoints:
            fd.write(
                "<details>\n"
                "<summary><code>{}</code> <code><b>{}</b></code> <code>{}</code></summary>\n\n".format(
                    endpoint.method, endpoint.path, endpoint.summary))

            if endpoint.description:
                fd.write("##### Description\n\n{}.\n\n".format(endpoint.description))
            if endpoint.parameters:
                fd.write("##### Parameters\n\n")
                fd.write(make_markdown_table(
                    [("Name", "Located in", "Description", "Required", "Schema"),
                     *[(p.name, p.location, p.description, "Yes" if p.required else "No", p.schema)
                       for p in endpoint.parameters]]))
            if endpoint.responses:
                fd.write("##### Responses\n\n")
                fd.write(make_markdown_table(
                    [("Code", "Description"),
                     *[(r.code, r.description) for r in sorted(endpoint.responses, key=attrgetter("code"))]]))

            fd.write("</details>\n")
        fd.write("\n------------------------------------------------------------------------------------------\n")


def main():
    r = requests.get("http://localhost:8080/api-docs/oas-3.0.0.json")
    r.raise_for_status()
    spec = r.json()

    endpoints_by_section = OrderedDict({"Swagger": [
        Endpoint("/swagger/ui", "GET", "Access swagger interface", "Access swagger interface", [],
                 [Response("200", "OK")]),
        Endpoint("/api-docs/oas-3.0.0.json", "GET", "Get API definition", "Get openapi JSON API definition", [],
                 [Response("200", "OK")])
    ]})

    for endpoint in parse_oas(spec):
        section = get_section(endpoint.path)
        endpoints = endpoints_by_section.get(section)
        if endpoints:
            endpoints.append(endpoint)
        else:
            endpoints_by_section[section] = [endpoint]

    root_path = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
    with open(os.path.join(root_path, "docs", "rest-api.md"), "w") as f:
        create_md(f, endpoints_by_section, spec["info"]["version"], spec["info"]["description"])


if __name__ == "__main__":
    main()
