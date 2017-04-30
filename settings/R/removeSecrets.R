##' Takes in a list and a second list of items to remove from first
##' list.
##'
##' This will take two lists as input. The first list is the input
##' list, the second list is the list of all keywords to remove. All
##' names in the first list are converted to lowercase and if matched
##' in the second list they will be removed.
##'
##' @title Remove Secrets
##' @param input list that should be filtered.
##' @param secrets keywords to remove from the first list.
##' @return the input list with all the elements from the secrets
##'         removed.
##' @author Rob Kooper
##' @export removeSecrets
removeSecrets <- function(input, secrets = c('password')) {
  if (!is.list(input)) {
    return(input)
  } else {
    input[tolower(names(input)) %in% secrets] <- '*****'
    output <- lapply(input, removeSecrets, secrets)
    return(output)
  }
}
