import json, urllib.request

url = "https://api.github.com/repos/limine-bootloader/limine/releases/latest"
req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
with urllib.request.urlopen(req) as resp:
    data = json.load(resp)

print("Tag:", data["tag_name"])
for asset in data["assets"]:
    print(asset["name"], "->", asset["browser_download_url"])
