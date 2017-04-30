##' Takes in a list and checks if any value is '*****'.
##'
##' This will take list as input and checks if any value is '*****'.
##' This indicates it was a secret that was removed when saving the
##' file previously.
##'
##' @title Check Secrets
##' @param input list that should be check for '*****'
##' @return true if anywhere in the list '*****' is found
##' @author Rob Kooper
##' @export checkSecrets
checkSecrets <- function(input) {
  return(any(rapply(input, function(x)
    x == '*****', how = 'unlist')))
}
