#################
# Builder stage #
#################

FROM alpine:3.9 AS builder
LABEL maintainer="mark@flaneur.tv"
LABEL license=MIT

RUN apk update \
    && apk add --virtual build-dependencies \
        build-base \
        gcc \
        wget \
        git

COPY . /usr/src/

RUN cd /usr/src && \
  make && \
  cp client /usr/local/bin/prompeg && \
  chmod +x /usr/local/bin/prompeg


#################
# Runtime stage #
#################

FROM alpine:3.9 AS runtime
LABEL maintainer="mark@flaneur.tv"
LABEL license=MIT

RUN apk update \
  && apk add bash libstdc++ libgcc

COPY prompeg.sh /usr/local/bin/prompeg.sh
COPY --from=builder /usr/local/bin/prompeg /usr/local/bin/prompeg

ENTRYPOINT /usr/local/bin/prompeg
