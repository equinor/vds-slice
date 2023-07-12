package core

type InvalidArgument struct {
	message string
}

func (e *InvalidArgument) Error() string {
	return e.message
}

func NewInvalidArgument(msg string) *InvalidArgument {
	return &InvalidArgument{
		message: msg,
	}
}

type InternalError struct {
	message string
}

func (e *InternalError) Error() string {
	return e.message
}

func NewInternalError(msg string) *InternalError {
	return &InternalError{ message: msg }
}
