import re
import ast
import json
import tokenize
from io import StringIO


def generate_cpp_from_python(
    python_file_path, cpp_output_path, function_name, class_name
):
    """
    Generates a C++ class with a method translated from a Python function.

    :param python_file_path: Path to the Python file containing the function.
    :param cpp_output_path: Path where the generated C++ code will be saved.
    :param function_name: Name of the Python function to convert.
    :param class_name: Name of the C++ class to generate.
    """
    # Read the Python source code
    with open(python_file_path, "r") as file:
        python_code = file.read()

    # Extract comments with their line numbers
    comment_dict = extract_comments(python_code)

    # Parse the Python code into an AST
    tree = ast.parse(python_code)

    # Extract the specified function
    func_def = None
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == function_name:
            func_def = node
            break

    if not func_def:
        print(f"Function '{function_name}' not found in {python_file_path}.")
        return

    # Extract variables assigned from Z.*
    z_assignments = {}
    for node in func_def.body:
        if isinstance(node, ast.Assign):
            target = node.targets[0]
            if (
                isinstance(node.value, ast.Attribute)
                and isinstance(node.value.value, ast.Name)
                and node.value.value.id == "Z"
            ):
                var_name = target.id
                z_attr = node.value.attr
                z_assignments[var_name] = z_attr

    if not z_assignments:
        print(f"No assignments from 'Z' found in function '{function_name}'.")
        return

    # Initialize C++ code components
    includes = [
        '#include "PropagationModel.h"',
        '#include "FireDomain.h"',
        "#include <cmath>",
        "#include <iostream>",
        "using namespace std;",
        "",
    ]

    namespace = "libforefire"

    # Generate class member variables
    member_vars = ""
    for var, attr in z_assignments.items():
        member_vars += f"    size_t {var}_;\n"

    # Add any additional member variables if needed
    # For example, if 'slope_rad' is used, it should be defined or calculated in C++

    class_declaration = (
        f"class {class_name}: public PropagationModel {{\n"
        f"public:\n"
        f"    {class_name}(const int& = 0, DataBroker* db=0);\n"
        f"    virtual ~{class_name}();\n\n"
        f"    string getName();\n"
        f"    double getSpeed(double*);\n\n"
        f"private:\n"
        f"{member_vars}"
        f"    double windReductionFactor;\n"
        f"}};\n\n"
    )

    # Function implementation template
    function_implementation = (
        f"{class_name}::{class_name}(const int & mindex, DataBroker* db)\n"
        f": PropagationModel(mindex, db) {{\n"
        f"    /* Defining the properties needed for the model */\n"
        f'    windReductionFactor = params->getDouble("windReductionFactor");\n\n'
    )

    for var, attr in z_assignments.items():
        function_implementation += f'    {var}_ = registerProperty("fuel.{attr}");\n'

    function_implementation += (
        "\n    /* Allocating the vector for the values of these properties */\n"
        "    if (numProperties > 0) properties = new double[numProperties];\n\n"
        "    /* Registering the model in the data broker */\n"
        "    dataBroker->registerPropagationModel(this);\n\n"
        "    /* Definition of the coefficients */\n"
        "}\n\n"
        f"{class_name}::~{class_name}() {{\n"
        f"}}\n\n"
        f"string {class_name}::getName() {{\n"
        f'    return "{class_name}";\n'
        f"}}\n\n"
    )

    # Extract the function body and translate it
    class_method = translate_python_function_to_cpp(
        func_def, class_name, z_assignments, comment_dict
    )

    # Combine all parts
    cpp_code = (
        "\n".join(includes)
        + f"namespace {namespace} {{\n\n"
        + class_declaration
        + function_implementation
        + class_method
        + "}\n"
    )

    # Write to the output C++ file
    with open(cpp_output_path, "w") as cpp_file:
        cpp_file.write(cpp_code)

    print(f"C++ code has been generated and saved to '{cpp_output_path}'.")


def extract_comments(python_code):
    """
    Extracts comments from Python code along with their line numbers.

    :param python_code: The entire Python source code as a string.
    :return: A dictionary mapping line numbers to comments.
    """
    comment_dict = {}
    f = StringIO(python_code)
    for toknum, tokval, (srow, scol), (erow, ecol), _ in tokenize.generate_tokens(
        f.readline
    ):
        if toknum == tokenize.COMMENT:
            # Store comment without the '#' symbol and strip leading whitespace
            comment_text = tokval.lstrip("#").strip()
            comment_dict[srow] = comment_text
    return comment_dict


def translate_python_function_to_cpp(func_def, class_name, z_assignments, comment_dict):
    """
    Translates a Python function AST into a C++ class method, preserving comments and enhancing return statements.

    :param func_def: AST node of the Python function.
    :param class_name: Name of the C++ class.
    :param z_assignments: Dictionary mapping Python variables to Z attributes.
    :param comment_dict: Dictionary mapping line numbers to comments.
    :return: String containing the C++ method implementation.
    """
    # Initialize the C++ method
    method_signature = f"double {class_name}::getSpeed(double* valueOf) {{\n"
    method_body = ""
    indent = "    "

    # Declare variables from valueOf
    for var in z_assignments:
        method_body += f"{indent}double {var} = valueOf[{var}_];\n"

    # Retrieve the source lines of the function for comment mapping
    func_source_lines = get_function_source_lines(func_def)

    # Traverse the function body
    for node in func_def.body:
        translated_code = translate_ast_node_with_comments(
            node, indent, comment_dict, func_def.lineno
        )
        method_body += translated_code

    method_body += "}\n\n"
    return method_signature + method_body


def get_function_source_lines(func_def):
    """
    Retrieves the source lines of the Python function.

    :param func_def: AST node of the Python function.
    :return: List of source lines.
    """
    if hasattr(func_def, "body") and func_def.body:
        # Assuming the function is part of a parsed AST from the entire file
        # There's no direct way to get source lines from AST nodes
        # So we skip this function for simplicity
        return []
    return []


def translate_ast_node_with_comments(node, indent, comment_dict, func_start_lineno):
    """
    Translates an AST node into C++ code, including comments.

    :param node: AST node.
    :param indent: Current indentation.
    :param comment_dict: Dictionary mapping line numbers to comments.
    :param func_start_lineno: Starting line number of the function.
    :return: Translated C++ code as a string.
    """
    translated_code = ""
    # Calculate the actual line number of the node
    lineno = getattr(node, "lineno", None)
    if lineno and lineno in comment_dict:
        comment = comment_dict[lineno]
        translated_code += f"{indent}/* {comment} */\n"

    if isinstance(node, ast.Assign):
        # Skip assignments from Z.*, already handled
        if (
            isinstance(node.value, ast.Attribute)
            and isinstance(node.value.value, ast.Name)
            and node.value.value.id == "Z"
        ):
            return translated_code
        translated_code += translate_assign(node, indent)
    elif isinstance(node, ast.If):
        translated_code += translate_if_with_comments(
            node, indent, comment_dict, func_start_lineno
        )
    elif isinstance(node, ast.Expr):
        translated_code += translate_expr_with_comments(
            node, indent, comment_dict, func_start_lineno
        )
    elif isinstance(node, ast.Return):
        translated_code += translate_return_with_comments(
            node, indent, comment_dict, func_start_lineno
        )
    else:
        translated_code += f"{indent}// Unsupported node type: {type(node)}\n"
    return translated_code


def translate_assign(node, indent):
    """
    Translates an assignment AST node to C++.

    :param node: AST Assign node.
    :param indent: Current indentation.
    :return: C++ code string.
    """
    # Assuming single target
    target = node.targets[0]
    target_code = translate_expr_to_cpp(target)
    value_code = translate_expr_to_cpp(node.value)

    # Handle variable declarations
    # For simplicity, declare all as double
    cpp_line = f"{indent}double {target_code} = {value_code};\n"
    return cpp_line


def translate_if_with_comments(node, indent, comment_dict, func_start_lineno):
    """
    Translates an if AST node to C++, including comments.

    :param node: AST If node.
    :param indent: Current indentation.
    :param comment_dict: Dictionary mapping line numbers to comments.
    :param func_start_lineno: Starting line number of the function.
    :return: C++ code string.
    """
    cpp = f"{indent}if ({translate_expr_to_cpp(node.test)}) {{\n"
    for stmt in node.body:
        cpp += translate_ast_node_with_comments(
            stmt, indent + "    ", comment_dict, func_start_lineno
        )
    cpp += f"{indent}}}\n"

    # Handle elif and else
    for orelse in node.orelse:
        if isinstance(orelse, ast.If):
            cpp += f"{indent}else if ({translate_expr_to_cpp(orelse.test)}) {{\n"
            for stmt in orelse.body:
                cpp += translate_ast_node_with_comments(
                    stmt, indent + "    ", comment_dict, func_start_lineno
                )
            cpp += f"{indent}}}\n"
        else:
            cpp += f"{indent}else {{\n"
            cpp += translate_ast_node_with_comments(
                orelse, indent + "    ", comment_dict, func_start_lineno
            )
            cpp += f"{indent}}}\n"
    return cpp


def translate_expr_with_comments(node, indent, comment_dict, func_start_lineno):
    """
    Translates an expression AST node to C++, including comments.

    :param node: AST Expr node.
    :param indent: Current indentation.
    :param comment_dict: Dictionary mapping line numbers to comments.
    :param func_start_lineno: Starting line number of the function.
    :return: C++ code string.
    """
    # Handle print statements
    if (
        isinstance(node.value, ast.Call)
        and isinstance(node.value.func, ast.Name)
        and node.value.func.id == "print"
    ):
        args = node.value.args
        cpp_line = f"{indent}std::cout"
        for arg in args:
            cpp_line += f' << {translate_expr_to_cpp(arg)} << " "'
        cpp_line += " << std::endl;\n"
        return cpp_line
    else:
        return f"{indent}// Unsupported expression\n"


def translate_return_with_comments(node, indent, comment_dict, func_start_lineno):
    """
    Translates a return AST node to C++, including detailed comments for complex returns.

    :param node: AST Return node.
    :param indent: Current indentation.
    :param comment_dict: Dictionary mapping line numbers to comments.
    :param func_start_lineno: Starting line number of the function.
    :return: C++ code string.
    """
    if node.value:
        if isinstance(node.value, ast.Dict):
            # Convert the dictionary to a JSON-like string for the comment
            dict_content = {}
            for key, value in zip(node.value.keys, node.value.values):
                key_cpp = translate_expr_to_cpp(key)
                value_cpp = translate_expr_to_cpp(value)
                dict_content[key_cpp] = value_cpp
            dict_str = json.dumps(dict_content)
            return f"{indent}return /* Unsupported, python : {dict_str} */;\n"
        else:
            return_expr = translate_expr_to_cpp(node.value)
            return f"{indent}return {return_expr};\n"
    else:
        return f"{indent}return;\n"


def translate_expr_to_cpp(expr):
    """
    Recursively translates a Python AST expression to C++.

    :param expr: AST expression node.
    :return: C++ expression as string.
    """
    if isinstance(expr, ast.BinOp):
        left = translate_expr_to_cpp(expr.left)
        right = translate_expr_to_cpp(expr.right)
        op = translate_operator(expr.op)
        if isinstance(expr.op, ast.Pow):
            return f"pow({left}, {right})"
        else:
            return f"({left} {op} {right})"
    elif isinstance(expr, ast.UnaryOp):
        operand = translate_expr_to_cpp(expr.operand)
        op = translate_unary_operator(expr.op)
        return f"({op}{operand})"
    elif isinstance(expr, ast.Call):
        func = translate_function_call(expr.func)
        args = ", ".join([translate_expr_to_cpp(arg) for arg in expr.args])
        return f"std::{func}({args})"
    elif isinstance(expr, ast.Num):
        return str(expr.n)
    elif isinstance(expr, ast.Name):
        return expr.id
    elif isinstance(expr, ast.Attribute):
        # For expressions like math.radians
        if isinstance(expr.value, ast.Name) and expr.value.id == "math":
            return f"std::{expr.attr}"
        else:
            return f"{translate_expr_to_cpp(expr.value)}.{expr.attr}"
    elif isinstance(expr, ast.Compare):
        left = translate_expr_to_cpp(expr.left)
        comparators = [translate_expr_to_cpp(comp) for comp in expr.comparators]
        ops = [translate_comparator(op) for op in expr.ops]
        comparison = " ".join([f"{ops[i]} {comparators[i]}" for i in range(len(ops))])
        return f"({left} {comparison})"
    elif isinstance(expr, ast.BoolOp):
        # Handle 'and', 'or'
        op = translate_bool_operator(expr.op)
        values = [translate_expr_to_cpp(v) for v in expr.values]
        return f"({f' {op} '.join(values)})"
    elif isinstance(expr, ast.IfExp):
        # Ternary operator
        test = translate_expr_to_cpp(expr.test)
        body = translate_expr_to_cpp(expr.body)
        orelse = translate_expr_to_cpp(expr.orelse)
        return f"({test} ? {body} : {orelse})"
    elif isinstance(expr, ast.Constant):  # For Python 3.8+
        return str(expr.value)
    else:
        return "/* Unsupported expression */"


def translate_operator(op):
    """
    Translates a Python binary operator to C++.

    :param op: AST operator.
    :return: C++ operator as string.
    """
    operators = {
        ast.Add: "+",
        ast.Sub: "-",
        ast.Mult: "*",
        ast.Div: "/",
        ast.Mod: "%",
        ast.LShift: "<<",
        ast.RShift: ">>",
        ast.BitOr: "|",
        ast.BitXor: "^",
        ast.BitAnd: "&",
        ast.FloorDiv: "/",
    }
    return operators.get(type(op), "?")


def translate_unary_operator(op):
    """
    Translates a Python unary operator to C++.

    :param op: AST unary operator.
    :return: C++ operator as string.
    """
    operators = {ast.UAdd: "+", ast.USub: "-", ast.Not: "!", ast.Invert: "~"}
    return operators.get(type(op), "?")


def translate_comparator(op):
    """
    Translates a Python comparator to C++.

    :param op: AST comparator.
    :return: C++ comparator as string.
    """
    comparators = {
        ast.Eq: "==",
        ast.NotEq: "!=",
        ast.Lt: "<",
        ast.LtE: "<=",
        ast.Gt: ">",
        ast.GtE: ">=",
        ast.Is: "==",
        ast.IsNot: "!=",
        ast.In: "==",  # Simplification
        ast.NotIn: "!=",  # Simplification
    }
    return comparators.get(type(op), "?")


def translate_bool_operator(op):
    """
    Translates a Python boolean operator to C++.

    :param op: AST boolean operator.
    :return: C++ boolean operator as string.
    """
    bool_ops = {ast.And: "&&", ast.Or: "||"}
    return bool_ops.get(type(op), "?")


def translate_function_call(func):
    """
    Translates a Python function call to C++.

    :param func: AST function node.
    :return: C++ function name as string.
    """
    if isinstance(func, ast.Attribute):
        # e.g., math.radians -> radians
        return func.attr
    elif isinstance(func, ast.Name):
        return func.id
    else:
        return "/* Unsupported function */"


# Example usage:
""" generate_cpp_from_python(
    "RothermelAndrews2018.py",
    "RothermelAndrews2018.cpp",
    "RothermelAndrews2018",
    "RothermelAndrews2018",
) """
