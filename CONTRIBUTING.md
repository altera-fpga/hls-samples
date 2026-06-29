# Contributing to hls-samples

The `main` branch contains code samples that work with the latest version of the Altera® HLS IP Gen compiler. If you want to contribute to the product, it is best to open a pull request (PR) to this branch. PRs will be reviewed before being merged.

## Fork the Repository

1. To fork the repository from the GitHub user interface, click the **Fork** icon then select **Create a new fork**. The fork will be created in few seconds. If you previously forked the repo, skip to the Step 5.

2. Select an **Owner** for the forked repository.

3. Deselect the **Copy the main branch only** check box. (It should be unchecked before proceeding to the next step.)

4. Click the **Create fork** button.

5. If you have an existing fork but do not have a `main` branch, create a `main` branch by selecting the altera-fpga/hls-samples `main` branch in the dropdown as the branch source.

6. Once your fork has been created, click the **Settings** icon and find the **Default Branch** section.

7. Click the **Switch to another branch** graphic.

8. From the dropdown, change the default branch to `main`. Click the **Update** button.

9. To create a branch in your fork, make sure the `main` branch is selected from the dropdown, and enter the name of your branch in the text field.

## Clone Your Fork

Clone the repo and checkout the branch that you just created by entering a command similar to the following:

```
git clone -b <your branch name> https://github.com/<your GitHub username>/<your repo name>.git
```

Once you are ready to commit your changes to your repo, enter commands similar to the following:

```
git add .
git commit -s -m "<insert commit reason here>"
git push origin
```

## Submit Pull Requests

When submitting a pull request, keep the following guidelines in mind:

- Make sure that your pull request has a clear purpose; it should be as simple as possible. This approach enables quicker PR reviews.

- Explain anything non-obvious from the code in comments, commit messages, or the PR description, as needed.

- Check the number of files being updated. Ensure that your pull request includes only the files you expected to be changed. (If there are additional files you did not expect included in the commit, troubleshoot before submitting the PR.)

## Log a Bug or Request a Feature

We use [GitHub Issues](https://github.com/altera-fpga/hls-samples/issues) to track sample development issues, bugs, and feature requests.

When reporting a bug, provide the following information when possible:

- Steps to reproduce the bug.
- Whether you found or reproduced the bug using the latest sample in the `main` branch and the latest compatible Altera® HLS IP Gen compiler version.
- Version numbers or other information about the FPGA, platform, operating system or distribution you used to find the bug.

## License

Code samples in this repository are licensed under the terms outlined in [License.txt](https://github.com/altera-fpga/hls-samples/blob/main/License.txt). By contributing to the project, you agree to the license and copyright terms therein and release your contribution under these terms.
