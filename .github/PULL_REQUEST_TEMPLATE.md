Erase this information and put your Pull Request comments here 
---

Instructions for Issuing a Pull Request to sst-elements
-------------------------------------------------------

1 - Verify that the Pull Request is targeted to the **devel** branch of sstsimulator/sst-elements

2 - Verify that Source branch is up to date with the devel branch of sst-elements

3 - After submitting your Pull Request:
   * Automatic Testing will commence in a short while 
      * Pull Requests will be tested with the devel branches of the sst-core and sst-sqe repositories
         * These branches are syncronized with the devel branch of sst-elements.  This is why is it important to keep your source branch up to date.
      * If testing passes, the source branch will be automatically merged (if possible)
         * Pull Requests from forks will not be automatically tested until the code is inspected.
         * Pull Requests from forks will not be automatically merged into the devel branch.
      * If testing fails, You will be notified of the test results.  
         * The Pull Request will be retested on a regular basis - Changes to the source branch can be made to correct problems
         
4 - DO NOT DELETE THE BRANCH (OR FORKED REPO) UNTIL THE PULL REQUEST IS MERGED.
----
