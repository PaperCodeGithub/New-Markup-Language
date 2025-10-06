import xml.etree.ElementTree as ET
import json

# Recursive function to convert XML element to AST node
def element_to_ast(elem):
    node = {
        "tag": elem.tag,
        "attrs": elem.attrib.copy(),
        "children": []
    }
    # If element has text that's not just whitespace, store it
    text = elem.text.strip() if elem.text else ""
    if text:
        node["children"].append(text)
    
    # Recurse on children
    for child in elem:
        node["children"].append(element_to_ast(child))
    return node

def winml_to_json_ast(winml_file, output_file):
    tree = ET.parse(winml_file)
    root = tree.getroot()
    ast = element_to_ast(root)
    with open(output_file, "w") as f:
        json.dump(ast, f, indent=4)
    print(f"AST saved to {output_file}")

if __name__ == "__main__":
    name = input(">> ")
    winml_file = name     # your WinML file
    output_file = "ast.json" # output JSON AST
    winml_to_json_ast(winml_file, output_file)
