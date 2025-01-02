# convert old level format to new
from glob import glob
import json

if __name__ == "__main__":
    current_base = None
    num_bases = 0
    for file in glob("./*.info"):
        print(f"opening {file}")
        with open(file) as f_handle:
            output = {"base_list":[]}
            for line in f_handle.readlines():
                if line.startswith("end"):
                    break
                key,value = line.split('=')
                value = value.strip()
                if current_base and num_bases != 0:
                    if key == "basex":
                        current_base["x"] = int(value)
                    elif key == "basey":
                        current_base["y"] = int(value)
                    if "x" in current_base and "y" in current_base:
                        num_bases = num_bases - 1
                        output["base_list"].append(current_base)

                        current_base = {"team":current_base["team"]}
                    continue

                if key in ("basesblue", "basesred", "basesneutral"):
                    current_base = {"team": key[5:]}
                    num_bases = int(value)
                    continue
                if key in ("version", "bases"):
                    output[key] = int(value)
                else:
                    output[key] = value

            with open(file.replace(".info", ".json"),"w+") as out_file: 
                out_file.write(json.dumps(output, indent=2))
