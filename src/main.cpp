#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

/**
 * @brief Removes leading and trailing whitespace characters from the given string.
 *
 * Trims spaces, tabs, and newline-related characters at both ends. If the string
 * is composed entirely of whitespace, returns an empty string.
 *
 * @param s The input string to be trimmed.
 * @return A new string with leading/trailing whitespace removed.
 */
std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        // No non-whitespace characters found.
        return "";
    }
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

/**
 * @brief Splits a string by commas into a list of trimmed tokens.
 *
 * Each token is trimmed of leading/trailing whitespace. Empty tokens are discarded.
 *
 * Example: "  foo, bar,  baz " -> ["foo", "bar", "baz"]
 *
 * @param input The comma-separated string.
 * @return A vector of trimmed tokens.
 */
std::vector<std::string> splitByComma(const std::string &input)
{
    std::vector<std::string> items;
    std::istringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (!item.empty()) {
            items.push_back(item);
        }
    }
    return items;
}

/**
 * @brief Formats a list of package names to display in quotes. If multiple
 *        names, encloses them in parentheses. Example: ( "pkg1" "pkg2" ).
 *
 * If there's exactly one package, returns simply:  "pkg"
 * If empty, returns an empty string "".
 *
 * @param packages A vector of package names.
 * @return A formatted string representation of the package list.
 */
std::string formatPackageNames(const std::vector<std::string> &packages)
{
    if (packages.empty()) {
        return "";
    }
    if (packages.size() == 1) {
        // One package, e.g. "\"mypackage\""
        return "\"" + packages[0] + "\"";
    }

    // Multiple packages, e.g. ( "pkg1" "pkg2" )
    std::string result = "( ";
    for (const auto &pkg : packages) {
        result += "\"" + pkg + "\" ";
    }
    result += ")";
    return result;
}

/**
 * @brief Converts a comma-separated list into a parenthesized array of quoted items.
 *
 * Example: "foo,bar" -> ( "foo" "bar" )
 *
 * @param input A comma-separated string of items.
 * @return The formatted array string, or "( )" if no tokens are found.
 */
std::string formatArray(const std::string &input)
{
    auto items = splitByComma(input);
    if (items.empty()) {
        return "( )";
    }

    std::string result = "( ";
    for (const auto &item : items) {
        result += "\"" + item + "\" ";
    }
    result += ")";
    return result;
}

/**
 * @brief Reads multiple lines from stdin until a line containing only "END"
 *        is encountered, returning the accumulated lines as a single string.
 *
 * @param prompt A message displayed to the user before reading lines.
 * @return A string containing all entered lines (excluding the "END" line).
 */
std::string readMultiline(const std::string &prompt)
{
    std::cout << prompt << " (end with a line containing only END):\n";

    std::string line, result;
    while (std::getline(std::cin, line)) {
        if (trim(line) == "END") {
            break;
        }
        result += line + "\n";
    }
    return result;
}

int main()
{
    // ---------------------------------------------------------------------
    // 1.  Gather core package info
    // ---------------------------------------------------------------------
    std::string pkgNamesInput, pkgVersion;
    std::string globalDependencies, globalBuildDependencies, sources;

    std::cout << "Enter package name(s) (comma separated): ";
    std::getline(std::cin, pkgNamesInput);
    pkgNamesInput = trim(pkgNamesInput);

    std::vector<std::string> packageNames = splitByComma(pkgNamesInput);

    std::cout << "Enter package version: ";
    std::getline(std::cin, pkgVersion);
    pkgVersion = trim(pkgVersion);

    // Per‑package descriptions / extra deps
    std::vector<std::string> packageDescriptions;
    std::vector<std::string> packageSpecificDeps;

    if (packageNames.size() == 1) {
        std::string desc;
        std::cout << "Enter package description: ";
        std::getline(std::cin, desc);
        packageDescriptions.push_back(trim(desc));
    } else {
        for (const auto &pkg : packageNames) {
            std::string desc;
            std::cout << "Enter description for package \"" << pkg << "\": ";
            std::getline(std::cin, desc);
            packageDescriptions.push_back(trim(desc));

            std::string pkgDeps;
            std::cout << "Enter additional dependencies for \"" << pkg
                      << "\" (comma separated, or leave blank): ";
            std::getline(std::cin, pkgDeps);
            packageSpecificDeps.push_back(trim(pkgDeps));
        }
    }

    // ---------------------------------------------------------------------
    // 2.  Global relationships (dependencies, clashes, gives, etc.)
    // ---------------------------------------------------------------------
    std::cout << "Enter global dependencies (comma separated, or leave blank): ";
    std::getline(std::cin, globalDependencies);

    std::cout << "Enter global build dependencies (comma separated, or leave blank): ";
    std::getline(std::cin, globalBuildDependencies);

    // NEW --------------------------------------------------------------
    std::string globalClashes, globalGives, optionalDeps;
    std::cout << "Enter clashes (comma separated, or leave blank): ";
    std::getline(std::cin, globalClashes);

    std::cout << "Enter gives (comma separated, or leave blank): ";
    std::getline(std::cin, globalGives);

    std::cout << "Enter optional dependencies (comma separated, or leave blank): ";
    std::getline(std::cin, optionalDeps);
    // ------------------------------------------------------------------

    std::cout << "Enter sources (comma separated, or leave blank): ";
    std::getline(std::cin, sources);

    // ---------------------------------------------------------------------
    // 3.  Build scripts
    // ---------------------------------------------------------------------
    std::string prepareScript = readMultiline("Enter prepare() script");
    std::string compileScript = readMultiline("Enter compile() script");
    std::string verifyScript  = readMultiline("Enter verify() script");

    std::vector<std::string> assembleScripts;
    if (packageNames.size() == 1) {
        assembleScripts.push_back(readMultiline("Enter assemble() script"));
    } else {
        for (const auto &pkg : packageNames) {
            assembleScripts.push_back(readMultiline("Enter assemble_" + pkg + "() script"));
        }
    }

    // ---------------------------------------------------------------------
    // 4.  Format everything into STARBUILD
    // ---------------------------------------------------------------------
    std::string formattedPkgNames     = formatPackageNames(packageNames);
    std::string globalDepsArray       = formatArray(globalDependencies);
    std::string globalBuildDepsArray  = formatArray(globalBuildDependencies);
    std::string sourcesArray          = formatArray(sources);

    // NEW arrays
    std::string clashesArray          = formatArray(globalClashes);
    std::string givesArray            = formatArray(globalGives);
    std::string optionalDepsArray     = formatArray(optionalDeps);

    std::stringstream starbuildContent;
    starbuildContent << "# STARBUILD generated by CLI tool\n\n";

    // Basic fields
    starbuildContent << "package_name="     << formattedPkgNames << "\n";
    starbuildContent << "package_version=\"" << pkgVersion << "\"\n";

    // Descriptions
    if (packageNames.size() == 1) {
        starbuildContent << "description=\"" << packageDescriptions[0] << "\"\n\n";
    } else {
        starbuildContent << "package_descriptions=( ";
        for (const auto &d : packageDescriptions)
            starbuildContent << "\"" << d << "\" ";
        starbuildContent << ")\n\n";
    }

    // Global arrays
    starbuildContent << "dependencies="          << globalDepsArray      << "\n";
    starbuildContent << "build_dependencies="    << globalBuildDepsArray << "\n";
    starbuildContent << "clashes="               << clashesArray         << "\n";        // NEW
    starbuildContent << "gives="                 << givesArray           << "\n";        // NEW
    starbuildContent << "optional_dependencies=" << optionalDepsArray    << "\n";        // NEW
    starbuildContent << "sources="               << sourcesArray         << "\n\n";

    // Scripts
    starbuildContent << "prepare() {\n" << prepareScript << "}\n\n";
    starbuildContent << "compile() {\n" << compileScript << "}\n\n";
    starbuildContent << "verify() {\n"  << verifyScript  << "}\n\n";

    if (packageNames.size() == 1) {
        starbuildContent << "assemble() {\n" << assembleScripts[0] << "}\n";
    } else {
        for (size_t i = 0; i < packageNames.size(); ++i) {
            starbuildContent << "assemble_" << packageNames[i] << "() {\n"
                             << assembleScripts[i] << "}\n\n";
        }
    }

    // Per‑package extra deps
    if (packageNames.size() > 1) {
        for (size_t i = 0; i < packageNames.size(); ++i) {
            if (!packageSpecificDeps[i].empty()) {
                starbuildContent << "dependencies_" << packageNames[i] << "="
                                 << formatArray(packageSpecificDeps[i]) << "\n";
            }
        }
    }

    // ---------------------------------------------------------------------
    // 5.  Write file
    // ---------------------------------------------------------------------
    std::ofstream outFile("STARBUILD");
    if (!outFile) {
        std::cerr << "Error: Could not write STARBUILD\n";
        return 1;
    }
    outFile << starbuildContent.str();
    outFile.close();

    std::cout << "STARBUILD file created successfully.\n";
    return 0;
}